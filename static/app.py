#!/usr/bin/env python3
"""
NutShell Search - HTTP 桥接服务器

功能：
  1. 托管 static/ 目录下的前端静态文件
  2. 将 HTTP API 请求翻译为 TLV 协议，转发给 muduo 服务器

启动方式：
  python3 app.py          # 默认端口 8080
  python3 app.py 9090     # 自定义端口

依赖：
  仅使用 Python 标准库，无需额外安装
"""

import http.server
import json
import os
import socket
import struct
import sys
import urllib.parse

# ============================================================
# TLV 协议常量
# ============================================================
TYPE_KEYWORD = 1   # 关键字推荐
TYPE_SEARCH  = 2   # 网页搜索

# muduo 服务器地址
MUDUO_HOST = '127.0.0.1'
MUDUO_PORT = 8888

# ============================================================
# TLV 协议编解码
# ============================================================
def encode_tlv(msg_type: int, value: str) -> bytes:
    """编码：type(1B) + length(4B, 大端) + value(UTF-8)"""
    body = value.encode('utf-8')
    header = struct.pack('!BI', msg_type, len(body))  # ! = 网络序(大端)
    return header + body


def decode_tlv(data: bytes):
    """解码 TLV 包，返回 (type, value_str)"""
    if len(data) < 5:
        return None
    msg_type = data[0]
    length = struct.unpack('!I', data[1:5])[0]
    if len(data) < 5 + length:
        return None
    value = data[5:5 + length].decode('utf-8')
    return msg_type, value


# ============================================================
# 向 muduo 服务器发送 TLV 请求并接收响应
# ============================================================
def query_muduo(msg_type: int, query_text: str, timeout: float = 5.0) -> str:
    """
    连接 muduo 服务器，发送 TLV 请求，接收 TLV 响应，
    返回解码后的 JSON 字符串。
    """
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        sock.connect((MUDUO_HOST, MUDUO_PORT))

        # 发送 TLV 请求包
        request_packet = encode_tlv(msg_type, query_text)
        sock.sendall(request_packet)

        # 接收响应（先收头部 5 字节）
        response_buffer = b''
        while True:
            try:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response_buffer += chunk

                # 尝试解码
                result = decode_tlv(response_buffer)
                if result is not None:
                    rtype, rvalue = result
                    sock.close()
                    return rvalue
            except socket.timeout:
                break

        sock.close()
        if response_buffer:
            return response_buffer.decode('utf-8', errors='replace')
        return json.dumps({"error": "服务器无响应"}, ensure_ascii=False)

    except socket.timeout:
        return json.dumps({"error": "连接 muduo 服务器超时，请确认 online_search 已启动"}, ensure_ascii=False)
    except ConnectionRefusedError:
        return json.dumps({"error": "无法连接 muduo 服务器 (127.0.0.1:8888)，请先启动 ./online_search"}, ensure_ascii=False)
    except Exception as e:
        return json.dumps({"error": f"通信异常: {str(e)}"}, ensure_ascii=False)


# ============================================================
# HTTP 请求处理器
# ============================================================
class SearchHandler(http.server.SimpleHTTPRequestHandler):
    """自定义 HTTP 请求处理器"""

    def __init__(self, *args, **kwargs):
        # 设置静态文件根目录为 static/ 所在目录
        super().__init__(*args, directory=os.path.dirname(os.path.abspath(__file__)), **kwargs)

    def do_GET(self):
        """处理 GET 请求：返回静态文件"""
        # 默认首页
        if self.path == '/' or self.path == '':
            self.path = '/index.html'
        super().do_GET()

    def do_POST(self):
        """处理 POST 请求：API 端点"""
        # 读取请求体
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length).decode('utf-8')

        try:
            data = json.loads(body)
            query = data.get('query', '').strip()
        except json.JSONDecodeError:
            self._send_json(400, {"error": "请求体必须是有效的 JSON"})
            return

        if not query:
            self._send_json(400, {"error": "请提供查询关键词"})
            return

        # 路由分发
        if self.path == '/api/keyword':
            result_json = query_muduo(TYPE_KEYWORD, query)
        elif self.path == '/api/search':
            result_json = query_muduo(TYPE_SEARCH, query)
        else:
            self._send_json(404, {"error": f"未知的 API 端点: {self.path}"})
            return

        # 返回结果
        try:
            result_obj = json.loads(result_json)
            self._send_json(200, result_obj)
        except json.JSONDecodeError:
            # 如果 muduo 返回的不是合法 JSON（比如旧版文本格式），直接返回原始文本
            self._send_json(200, {"raw": result_json})

    def _send_json(self, status_code: int, data):
        """发送 JSON 响应"""
        body = json.dumps(data, ensure_ascii=False).encode('utf-8')
        self.send_response(status_code)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        """自定义日志格式"""
        print(f"[HTTP] {args[0]}")


# ============================================================
# 启动服务器
# ============================================================
if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080

    print("=" * 55)
    print("  NutShell Search - HTTP 桥接服务器")
    print("=" * 55)
    print(f"  前端页面: http://localhost:{port}")
    print(f"  API 端点:")
    print(f"    POST /api/keyword  →  关键字推荐 (TLV type=1)")
    print(f"    POST /api/search   →  网页搜索   (TLV type=2)")
    print(f"  后端服务器: {MUDUO_HOST}:{MUDUO_PORT}")
    print("=" * 55)
    print(f"\n请先在另一个终端启动 muduo 服务器:")
    print(f"  cd /home/zyt/Search-Engine && ./online_search\n")

    server = http.server.HTTPServer(('0.0.0.0', port), SearchHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n服务器已关闭")
        server.shutdown()
