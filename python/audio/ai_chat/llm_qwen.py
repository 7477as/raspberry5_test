# llm_qwen.py
import ollama
import config

class QwenLLM:
    def __init__(self):
        self.model = config.LLM_MODEL_NAME
        print(f"🔗 连接到 Ollama (模型: {self.model})...")
        try:
            # 简单测试连接
            ollama.list()
            print("✅ Ollama 连接成功")
        except Exception as e:
            print(f"❌ 无法连接到 Ollama: {e}")
            print("请确保已运行 'ollama serve' 且已下载模型")

    def chat(self, user_text):
        """发送文本给 Qwen 并获取回复"""
        if not user_text:
            return ""
        
        print(f"\n🧠 Qwen2 正在思考: '{user_text}' ...")
        
        try:
            # stream=False: 等待完整回复后再返回 (适合语音场景，避免断断续续)
            response = ollama.chat(model=self.model, messages=[
                {'role': 'user', 'content': user_text},
            ])
            
            reply = response['message']['content']
            print(f"🤖 Qwen2 回复: {reply}")
            return reply
            
        except Exception as e:
            print(f"❌ 调用 Qwen 失败: {e}")
            return "抱歉，我现在脑子有点乱，请稍后再试。"
