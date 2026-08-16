import asyncio
import websockets

async def echo(websocket):
    # 获取客户端请求的路径和IP
    path = websocket.request.path
    client_ip = websocket.remote_address
    
    # 路径校验
    if path != "/ws":
        print(f"[拒绝连接] 客户端 {client_ip} 请求了错误的路径: {path}")
        await websocket.close(code=1008, reason="Path must be /ws")
        return

    print(f"[连接建立] 客户端 {client_ip} 已连接到 {path}")
    
    try:
        # 持续监听收到的消息
        async for message in websocket:
            print(f"[收到消息] 来源 {client_ip}: {message}")
            await websocket.send(message)
            
    except websockets.exceptions.ConnectionClosedError as e:
        # 仅捕获“异常断开”的情况（例如拔掉网线、客户端强杀进程）
        print(f"[异常断开] 客户端 {client_ip} 连接异常断开，原因: {e}")
        
    finally:
        # 重点在这里：不管是正常退出还是异常断线，循环结束后必定会执行 finally
        print(f"[连接关闭] 客户端 {client_ip} 已经彻底断开")

async def main():
    # 绑定 0.0.0.0:8090
    async with websockets.serve(echo, "0.0.0.0", 8090):
        print("WebSocket Echo 服务器已启动 -> ws://0.0.0.0:8090/ws")
        await asyncio.Future()  # 永久运行

if __name__ == "__main__":
    asyncio.run(main())