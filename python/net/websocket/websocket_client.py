import asyncio
import websockets

async def chat_with_server():
    # 注意：URL 结尾加上了 /ws
    # 如果在局域网的其他电脑上测试，请将 127.0.0.1 改成 192.168.0.212
    uri = "ws://127.0.0.1:8090/ws"
    
    print(f"正在连接到 {uri} ...")
    
    try:
        # 建立连接
        async with websockets.connect(uri) as websocket:
            print("连接成功！(输入 'quit' 或 'q' 退出)")
            print("-" * 30)
            
            while True:
                # 提示用户输入
                message_to_send = input("你想发送什么: ")
                
                # 退出条件
                if message_to_send.lower() in ['quit', 'q', 'exit']:
                    print("退出客户端...")
                    break
                
                # 发送消息给服务端
                await websocket.send(message_to_send)
                print(f"[客户端 -> 服务端] 发送: {message_to_send}")
                
                # 等待并接收服务端的回复
                response = await websocket.recv()
                print(f"[服务端 -> 客户端] 接收: {response}")
                print("-" * 30)
                
    except websockets.exceptions.InvalidStatusCode as e:
        print(f"\n连接被拒绝！服务端返回了错误状态。可能路径不正确。详细信息: {e}")
    except websockets.exceptions.ConnectionClosedError as e:
        print(f"\n连接被异常关闭（可能请求了错误的路径）。错误码: {e.code}, 原因: {e.reason}")
    except ConnectionRefusedError:
        print("\n连接失败！请检查服务端是否已启动，以及 IP/端口 是否正确。")
    except websockets.exceptions.ConnectionClosed:
        print("\n与服务器的正常连接已断开。")
    except KeyboardInterrupt:
        print("\n强制退出客户端。")

if __name__ == "__main__":
    # 运行异步客户端
    asyncio.run(chat_with_server())