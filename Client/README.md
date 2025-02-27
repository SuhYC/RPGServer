# 클라이언트 파트 진행 상황
async/await, tcpClient 활용하여 서버와 통신 <br/>
서버 <-> 클라이언트 간의 패킷 전달 시 패킷 분할 현상 해결 <br/>

SignUp 결과 수신 및 패킷처리 까지 완료 (이후 팝업 메시지를 띄워 결과 확인.)<br/>

SignIn 결과 수신 및 정보 요청 후 수신 완료 (SignIn -> GetCharList -> GetCharInfo)<br/>

GetCharList로 CharCode 리스트를 수신한 뒤, 해당 정보로 다시 GetCharInfo 요청 (캐릭터가 아직 없는 계정은 바로 씬 전환.)<br/>

GetCharInfo로 수신한 정보를 저장한 뒤, 모든 CharList의 CharInfo를 수신 완료했으면 캐릭터 선택창으로 씬 전환. <br/>

CreateChar 과정 구현. (ReserveCharName -> CreateChar) ReserveCharName을 호출한 뒤 생성가능한 경우 생성여부 재확인 창을 띄운다. 재확인창에서 생성을 선택한 경우 최종적으로 캐릭터가 생성된다. <br/>

ReserveCharName으로 생성가능한 닉네임에 예약을 건 후 생성여부를 재확인한다. 생성창에서 닉네임을 입력하고 버튼을 누르면 호출한다. <br/>

CancelCharNameReserve로 예약한 닉네임의 예약을 취소할 수 있다. 재확인창에서 생성을 거부하는 경우 호출한다. <br/>

GetInventory로 특정 캐릭터의 인벤토리 정보를 가져올 수 있다. <br/>

SelectChar로 특정 캐릭터로 접속하도록 요청할 수 있다. <br/>
해당 캐릭터의 정보가 모두 수신되었는지 확인하고 해당 캐릭터의 CharInfo에 있는 LastMapCode를 조회하여 해당하는 씬으로 씬전환한다. <br/>

#### 게임씬으로 진입한 이후

MoveMap으로 서버에 맵 이동을 요청하고, 서버에서 MapEdge테이블을 확인해 정상적으로 이동가능한 요청이라면 승인한다. <br/>
클라이언트에서는 각 씬에 포탈 프리팹을 배치하고 public int로 선언된 toMapCode를 수정하여 이동할 수 있도록 만들어둔다. <br/>
이후 PlayerCharacter 스크립트에서 UpArrowKey를 확인하고 오브젝트와 겹쳐있는 포탈 오브젝트를 가져와 toMapCode를 조회하고 MoveMap요청을 서버로 송신한다.

GetSalesList로 특정 npccode에 맞는 구매목록을 서버에 요청할 수 있다. <br/>
서버에서는 MapNPCInfo테이블을 확인하여 현재 위치한 맵에서 해당 NPC정보를 받아도 되는지 확인하고 승인한다. <br/>
SalesList테이블을 확인하여 해당 NPC가 판매하는 아이템들을 조회하고, ItemTable테이블을 확인하여 아이템의 가격을 조회하여 JSON문자열로 만들어 클라이언트에 전송한다. <br/>
클라이언트에서는 각 씬에 npc 프리팹을 배치하고, public int로 선언된 npcCode를 수정하여 구매목록을 가져올 수 있도록 만들어둔다. <br/>

BuyItem으로 특정 npc를 통해 특정 아이템을 구매하도록 서버에 요청할 수 있다. <br/>
서버에서는 MapNPCInfo테이블을 확인하여 현재 위치한 맵에서 해당 NPC를 만날 수 있는지 확인하고, <br/>
SalesList테이블을 확인하여 해당 NPC가 해당 아이템을 판매하는지 확인하고, <br/>
해당 유저의 CharInfo와 Inventory를 Redis에서 분산락을 이용해 상호배제한 후 수정을 시도하여 구매할 수 있는지 확인하여 승인한다. <br/>
클라이언트에서는 npc프리팹을 클릭하여 ShopPanel을 열고, 해당 패널에서 ShopSlot을 클릭하여 구매요청을 할 수 있도록 만들어둔다. <br/>

Chat으로 같은 맵에 존재하는 유저가 말한 채팅을 수신할 수 있도록하고, 같은 맵에 존재하는 유저에게 채팅을 송신할 수 있도록 한다. <br/>

SetGold로 디버깅을 위해 현재 골드를 1만으로 수정할 수 있도록 한다. 당연히 서버와 클라이언트 모두 정보를 수정할 수 있도록 한다.

## 패킷 분할
[MTU](https://github.com/SuhYC/Lesson/blob/main/Network/MTU.md)관련 내용과 연계되는 것으로 추정 (혹은 TCP 로직 처리 중에 분할되었을수도 있고..) <br/>
도착한 패킷만으로 메시지를 식별할 수 없는 경우 잔여버퍼를 따로 구성하여 저장해두도록 했다. <br/>
이후 도착한 패킷과 잔여버퍼의 메시지를 합쳐 식별할 수 있는 크기가 되면 해당 부분을 잘라 처리하고 남은 부분은 다시 잔여버퍼에 저장한다. <br/>
식별할 수 있는 크기가 되는지 확인하기 위해 메시지 앞에 [```size```]와 같이 작성한다. (페이로드 부분의 크기를 ```size```부분에 넣는방식) <br/>

-20250121- 슬슬 데이터 크기가 커지면서 기존에 처리하지 않았던 부분의 문제 발생.<br/>
한번의 2개 이상의 메시지가 1회의 IO로 수신된 경우 각 메시지를 처리한 후 잔여메시지를 저장해야하는데 <br/>
1개의 메시지만 처리하고 잔여메시지를 저장하고 함수를 종료하는 문제가 있었음. <br/>
처리 완료.

## 비동기 락
C#의 일반적인 락은 비동기 함수와 사용하기에 문제가 많다. <br/>
async 함수는 await 구문을 만나면 해당 부분의 동작이 완료되기 전까지 보류상태로 전환한 뒤 빠져나와 다른 함수를 실행해야하기 때문에 사용할 수 없다. <br/>
잔여메시지 관련으로 lock을 걸고 싶은데 수신과 수신처리에 있어서 async메소드를 사용하고 싶기 때문에 다른 방법을 사용한다. <br/>
SemaphoreSlim에서는 비동기를 위한 락을 제공한다. <br/>
다만 lock처럼 구문을 벗어날 때 자동으로 해제해주지는 않으므로 IDisposable인터페이스를 상속하여 자동으로 해제할 수 있도록 한다.

## async/await를 통한 수신 시 주의사항
현재는 다음과 같은 코드로 네트워크 수신을 하고 있다. <br/>
```csharp
try
{
    while (true)
    {
        int bytesRead = await _stream.ReadAsync(_buffer);
        if (bytesRead > 0)
        {
            //string receivedData = Encoding.UTF8.GetString(_buffer, 0, bytesRead);
            string receivedData = Encoding.GetEncoding("euc-kr").GetString(_buffer, 0, bytesRead);

            Debug.Log($"NetworkManager::RecvMsg : 수신: {receivedData}");
            await ResHandler.Instance.HandlePacketByte(receivedData);
        }
        // 연결종료, 오류 발생
        else
        {
            break;
        }
    }
}
catch (Exception e)
{
    Debug.LogError($"NetworkManager::RecvMsg : {e.Message}");
}
```
다 좋은데 반복문 부분에서 생길 수 있는 문제가 하나 있다. <br/>
특정 응답처리를 끝내기 전에 다른 서버요청을 하고 그 요청이 완료될 때까지 대기하면 안된다는 것. <br/>
분명 반복문의 시작부분에서 데이터를 읽고, 반복문의 끝부분에서 ```await```문으로 응답처리를 하고 있다. <br/>
하나의 응답처리가 끝나야 반복문을 다시 돌며 수신작업을 한다는 얘기. <br/>
이걸 생각 못하고 한 함수에서 필요한 데이터가 미수신 상태인 경우 요청하고, 수신 후 데이터가 갱신될 때까지 반복문을 돌며 대기하는 함수를 짰다가 네트워크 수신이 먹통이 되어버렸다. <br/>
해당 로직을 수정할 마땅한 방법이 있는 것이 아니라면, 미리 데이터를 수신할 수 있도록 선행되는 동작에 Req함수를 걸어놓던지, <br/>
실행요청 전에 다른 선행동작이 없고 해당 함수에서 판단해야한다면 필요한 데이터 요청을 하고, 해당 동작을 재요청한 다음, 함수를 끝내버리자. <br/>
```
ex.
캐릭터 선택 요청;

...

캐릭터 선택 응답
{
  if(인벤토리 정보 없음)
  {
    인벤토리 정보 요청;
    캐릭터 선택 요청;
    return; // 캐릭터 선택 응답 종료 (실패 후 재요청)
  }

  캐릭터 선택 응답 처리;
  return; // 완료 (성공)
}
```
