
## 1) OS 

- Mac OS
- Ubnutu 20.04.4



## 2) dependency packege

- gcc 14.2
- cmake 3.22.3
- Ninja 1.10.0
- boost 1.79.0



## 3) Build (MacOS)

```shell
mkdir build
cd build
cmake .. && make
./visual_camp
```





## 4) Build with Docker(Ubuntu 20.04.4)

```shell
# Build Dockerfile
docker build . -t visual-camp

# Run Docker
docker run -it --name {cotainer_name} visual-camp

# exec file
cd /source/build/
./visual_camp
```





## 5) 클래스 설명

### 1. Node Class

- `node class` 는 `input`,  `output` 에 구현부를 담고있는 `class` 입니다. 설명란에 `output`은 0개 또는 1개 이므로 `unique_ptr<output>` 로 보관하고, 하나의 노드는 여러개의 노드를 연결할 수 있으므로 `input`은 `vector<std::unique_ptr<input>>` 로 보관하고 있습니다.
- Node에 `operator()` 함수 객체를 구현해서 `data type` 과 `parameter` 개수에 맞는 functor 계산을 합니다.
- `make_edge()` 함수로 `output` 의 `set<input> `컨테이너에 `input` 을 링크 해줍니다.
- `work()` 함수는 `thread pool`에서 `package_task` 에 등록하고 `promise`에 반환값을 `output`의 `send_data()` 함수로 등록해서 `future` 를 통해 반환값을 획득 합니다.



### 2. Input Class

- Input은 `data`를 `vector`로 담고 있는 클래스 입니다. 
- 수많은 데이터가 들어와서, 들어온 데이터 쌍마다 고유 id(UUID)로 부과해서 `unordred_map` 으로 데이터를 보관하고 있습니다.



### 3. OutPut Class

- `make_edge` 로 통해서 연결된 `input class` 들을 `set` 컨테이너로 관리 하는 클레스입니다.
- `thread pool` 에  `send_data()` 함수로 작업을 요청합니다.
- `input` 들에 가지고 있는 `data`들을 다 가져와서 `functor` 계산을 하고 연결된 다음 노드로 뿌려줍니다. 여기서 해당 `input`에 `data instance`에 소유권을 다음 노드로 넘겨주고, `input` 의`unordered_map`에 담겨있는 자원을 모조리 해체 합니다.



### 4.Data Class

- 데이터를 가지고 있는 클래스 입니다.
- 데이터는 `boost:variant` 로 `functor` 에  의존되는 모든 `type` 을  `vector`로 보관합니다.
- `shared_ptr` 로 자원을 공유하거나, 현재 노드에서 다음노드로 이동될때는 현재 노드에 소유권을 다음 노드로 이전하고, 현재 노드에 자원을 해제합니다.



### 5. ThreadPool

`output` 의 `send_data()` 를 `thread job` 에 등록하고 `wait` 하고 있는 `thread` 를 하나 깨워서 일을 시키는 클래스 입니다. 



