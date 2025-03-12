# RPGServer
cpp project.

## skills
C++ IOCP Server <br/>
C# Unity Client <br/>
Redis with hiredis <br/>
MSSQL with odbc <br/>
rapidJson <br/>
(to-do) openssl <br/>

## 설명 영상 링크
[YoutubeLink](https://youtu.be/9p8_nc-A0UU) <br/>
영상 설명 및 댓글에 타임스탬프 있음.

## 오브젝트풀 및 커넥션풀
메모리의 할당 및 해제에는 오버헤드가 발생한다. <br/>
객체풀을 이용하면 이러한 부분에서 성능향상을 얻을 수 있다. <br/>
하지만 동기화문제를 해결해야한다. 본 프로젝트에서는 ```Concurrency::concurrent_queue<T>```를 사용해 동기화하도록 한다.<br/>
커넥션풀도 마찬가지다. 매 요청마다 핸들을 할당하고 연결요청을 하는건 오버헤드가 크다. <br/>
Redis와 MSSQL을 연결할 커넥션을 미리 할당해두고 사용할 스레드에서 받아와 사용하고 반납하는 것으로 한다. <br/>

물론 명확히 말하면 사실 lock-free하게 객체풀을 사용한다고 하더라도 커넥션은 각각의 스레드가 독립적으로 소유하고 있는 것이 더 낫긴 하다.

## Cache Server
Redis를 사용한다. 레디스에 해당 정보가 있는지 확인한 후 없다면 MSSQL을 조회하도록 한다. <br/>
[기존에 계획하던 인증서버](https://github.com/SuhYC/Authentication_Server)의 형태;로그인상태정보를 MSSQL에 저장하는 것에서 Redis에 저장하는 것으로 변경.<br/>

## Redis 분산락
Redis는 트랜잭션의 개념이 없다. <br/>
여러 명령어를 일련의 과정으로 수행하는 동안 다른 요청이 데이터를 수정하지 않도록 상호배제가 필요한 상황이 생긴다. <br/>
이 때 NX 키워드를 사용해 특정 데이터가 조작중인지를 확인한다. <br/>
예를 들어, (charcode)CharInfo의 형태를 갖는 key로 Redis에 저장하고 있었다고 가정하자. <br/>
```1234CharInfo``` (charcode가 1234인 캐릭터의 캐릭터정보를 의미하는 key값이다.) 의 value를 수정하고 싶다면, <br/>
```lock1234CharInfo```의 key로 SET NX를 시도하고, 그 결과가 성공이라면, 수행하고자 했던 동작을 모두 수행하고, DEL key를 요청하여 락을 해제하면 된다. <br/>

단, 락을 걸고 수행하던 도중에 락을 해제하지 못하고 서버가 문제가 생기는 경우를 생각해보자. <br/>
이미 Redis서버에는 락에 대한 정보가 있는 상태로 락을 해제해줄 수 있는 서버는 남아있지 않다. <br/>
그러므로 PX (또는 EX)의 옵션으로 락을 생성해야 한다. (PX : 밀리초, EX : 초) <br/>
``` SET key value NX PX 10000```와 같은 요청을 보내면, 해당 key값으로 저장된 데이터가 없을 때만 10000밀리초 동안 유지되는 데이터를 저장한다. <br/>
만약 수행 도중 서버가 문제가 생기더라도 Redis에서 10초 후 락을 해제해준다는 얘기.

## Dead Reckoning
위치정보에 대한 마지막 패킷으로부터 예상되는 위치를 추측하여 현재 위치를 계산하는 방법. <br/>
위치정보패킷을 매 프레임마다 전달할 수 없으니 환경에 맞게 패킷 전달 간격을 조정하고 구현하면 된다. <br/>
정확하게는 동역학의 변위 공식을 적용하는 것이 맞겠으나, 가속도를 포함한 적분공식을 사용하기는 과투자인 것 같아 <br/>
간단하게나마 속도를 고정하고 ```pos + vel * elapsedTime```정도로만 적용해보자.<br/>
나중에 더 정확한 공식이 필요하면 수정하도록 한다.

## EUC-KR 인코딩
한글문자를 포함한 C++ <-> C# 통신을 위해 인코딩을 C++에 맞추어 EUC-KR로 통일하였다. <br/>
C#에서 전송시 인코딩을 변경하여 전송. <br/>
단, MSSQL ODBC에서 ```SQLExecDirect```함수가 ```SQLWCHAR[]``` 타입의 쿼리를 받으므로 해당 부분에서만 다시 UTF-16으로 인코딩변경하여 작성한다.

## C# Client async/await
네트워크 IO와 같은 대기작업에서 async/await를 사용하였다. <br/>
동시에 async/await환경에서 임계영역 문제가 생기지 않도록 SemaphoreSlim을 사용하여 비동기 락을 사용.

## Todo
```openssl```사용하여 전송데이터 암호화 <br/>
```base64```사용하여 암호화된 데이터 내의 NULL값 제거 <br/>
기존 작성했던 헤더 가져올 예정.
