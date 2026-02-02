import asyncio
import websockets
import json

async def test():
    uri = "ws://localhost:3000"
    async with websockets.connect(uri) as websocket:
        # 1. Wait for Hello
        greeting = await websocket.recv()
        print(f"< {greeting}")

        # 2. Send Infer Request
        req = {
            "op": "infer",
            "prompt": "Solve this: If I have 3 apples and I eat 1, how many are left? Explain why.",
            "params": {
                "temp": 0.8
            }
        }
        await websocket.send(json.dumps(req))
        print(f"> {req}")

        # 3. Listen for tokens
        while True:
            msg = await websocket.recv()
            data = json.loads(msg)
            op = data.get("op")
            
            if op == "token":
                print(data["content"], end="", flush=True)
            elif op == "end":
                print("\n[DONE]")
                print(f"STATS: {json.dumps(data.get('stats'), indent=2)}")
                break
            elif op == "metrics":
                # Ignore metrics during inference (they come every second)
                # Uncomment below to see metrics:
                # print(f"\n[METRICS] GPU: {data['gpu']['temp']}°C, VRAM: {data['gpu']['vram_used_mb']}MB", flush=True)
                pass
            elif op == "error":
                print(f"\n[ERROR] {data}")
                break
            else:
                print(f"< {msg}")

asyncio.run(test())
