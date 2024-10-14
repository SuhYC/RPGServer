# RPGServer
cpp project.

## skills
C++ IOCP Server <br/>
Redis with hiredis <br/>
MSSQL with odbc <br/>
(to-do) rapidJson <br/>
(to-do) openssl <br/>

## 메모리풀 및 커넥션풀
메모리의 할당 및 해제에는 오버헤드가 발생한다. <br/>
객체풀을 이용하면 이러한 부분에서 성능향상을 얻을 수 있다. <br/>
하지만 동기화문제를 해결해야한다. 본 프로젝트에서는 ```Concurrency::concurrent_queue<T>```를 사용해 동기화하도록 한다.<br/>
커넥션풀도 마찬가지다. 매 요청마다 핸들을 할당하고 연결요청을 하는건 오버헤드가 크다. <br/>
Redis와 MSSQL을 연결할 커넥션을 미리 할당해두고 사용할 스레드에서 받아와 사용하고 반납하는 것으로 한다.

## Cache Server
Redis를 사용한다. 레디스에 해당 정보가 있는지 확인한 후 없다면 MSSQL을 조회하도록 한다. <br/>
[기존에 계획하던 인증서버](https://github.com/SuhYC/Authentication_Server)의 형태;로그인상태정보를 MSSQL에 저장하는 것에서 Redis에 저장하는 것으로 변경.<br/>

## Todo
```rapidJson```사용하여 전송할 데이터 문자열화 <br/>
```openssl```사용하여 전송데이터 암호화 <br/>
```base64```사용하여 암호화된 데이터 내의 NULL값 제거 <br/>
셋 다 기존 작성했던 헤더 가져올 예정.
