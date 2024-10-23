#include <iostream>
#include "src/DAG.h"

template<typename T>
class current_unix_epoch_time {
public:
    static unsigned long long int get() {
        std::chrono::time_point<std::chrono::system_clock> t = std::chrono::system_clock::now();
        return std::chrono::duration_cast<T>(t.time_since_epoch()).count();
    }
};

std::size_t parallel_data_1_test(){
    Node A, B, C, D, E, F;

    A.make_edge(C.get_in_port(0)); // A -> C
    B.make_edge(C.get_in_port(1)); // B -> C
    B.make_edge(F.get_in_port(0)); // B -> F
    C.make_edge(D.get_in_port(0)); // C -> D
    C.make_edge(E.get_in_port(0)); // C -> E
    C.make_edge(F.get_in_port(1)); // C -> F

    std::string uuid = uuid::generate_uuid();

    std::shared_ptr<Data> data1 = std::make_shared<Data>(std::vector<send_value_type>{10.0, 4, 20.0, std::vector<float>{40, 2.2, 30.5}},
                                                         uuid);
    std::shared_ptr<Data> data2 = std::make_shared<Data>(std::vector<send_value_type>{2, 5, 6, std::vector<float>{9.5, 2.2, 9}},
                                                         uuid);

    A.get_in_port(0)->put_data(uuid, std::move(data1));
    B.get_in_port(0)->put_data(uuid, std::move(data2));

    ThreadPool pool(6); // A, B, C, D, E, F 쓰레드 가동
    std::vector<std::future<std::vector<send_value_type>>> futures;

    std::vector<std::vector<send_value_type>> temp;
    temp.reserve(6);

    auto start = current_unix_epoch_time<std::chrono::nanoseconds>::get();
    futures.emplace_back(pool.add_job([p = &A, id = uuid]()->auto{
        return p->work(id);
    }));
    futures.emplace_back(pool.add_job([p = &B, id = uuid]()->auto{
        return p->work(id);
    }));
    futures.emplace_back(pool.add_job([p = &C, id = uuid]()->auto{
        return p->work(id);
    }));
    futures.emplace_back(pool.add_job([p = &D, id = uuid]()->auto{
        return p->work(id);
    }));
    futures.emplace_back(pool.add_job([p = &E, id = uuid]()->auto{
        return p->work(id);
    }));
    futures.emplace_back(pool.add_job([p = &F, id = uuid]()->auto{
        return p->work(id);
    }));

    for (auto& f : futures) {
        temp.push_back(std::move(f.get()));
    }
    auto end = current_unix_epoch_time<std::chrono::nanoseconds>::get();
    for(const auto& v : temp){
        std::cout << v <<std::endl;
    }
    return end-start;

}

std::size_t parallel_data_2_test(){
    Node A, B, C, D, E, F;
    /**
    std::cout << "[A] " << &A << std::endl;
    std::cout << "[B] " << &B << std::endl;
    std::cout << "[C] " << &C << std::endl;
    std::cout << "[D] " << &D << std::endl;
    std::cout << "[E] " << &E << std::endl;
    std::cout << "[F] " << &F << std::endl;
    **/

    A.make_edge(C.get_in_port(0)); // A -> C
    B.make_edge(C.get_in_port(1)); // B -> C
    B.make_edge(F.get_in_port(0)); // B -> F
    C.make_edge(D.get_in_port(0)); // C -> D
    C.make_edge(E.get_in_port(0)); // C -> E
    C.make_edge(F.get_in_port(1)); // C -> F

    std::string uuid = uuid::generate_uuid();
    std::string uuid2 = uuid::generate_uuid();

    std::shared_ptr<Data> data1 = std::make_shared<Data>(std::vector<send_value_type>{10.0, 4, 20.0, std::vector<float>{40, 2.2, 30.5}},
                                                         uuid);
    std::shared_ptr<Data> data2 = std::make_shared<Data>(std::vector<send_value_type>{2, 5, 6, std::vector<float>{9.5, 2.2, 9}},
                                                         uuid);
    std::shared_ptr<Data> data3 = std::make_shared<Data>(std::vector<send_value_type>{3, 7, 10.0, std::vector<float>{1.1, 2.2, 3}},
                                                         uuid2);
    std::shared_ptr<Data> data4 = std::make_shared<Data>(std::vector<send_value_type>{6, 14, 12, std::vector<float>{2, 1, 4}},
                                                         uuid2);

    A.get_in_port(0)->put_data(uuid, std::move(data1));
    B.get_in_port(0)->put_data(uuid, std::move(data2));
    A.get_in_port(0)->put_data(uuid2, std::move(data3));
    B.get_in_port(0)->put_data(uuid2, std::move(data4));

    ThreadPool pool(6); // A, B, C, D, E, F 쓰레드 가동
    std::vector<std::future<std::vector<send_value_type>>> futures;
    std::vector<std::string> uuids = {uuid, uuid2};

    std::vector<std::vector<send_value_type>> temp;
    temp.reserve(12);

    auto start = current_unix_epoch_time<std::chrono::nanoseconds>::get();
    for(const std::string& u : uuids){
        futures.emplace_back(pool.add_job([p = &A, id = u]()->auto{
            return p->work(id);
        }));
        futures.emplace_back(pool.add_job([p = &B, id = u]()->auto{
            return p->work(id);
        }));
        futures.emplace_back(pool.add_job([p = &C, id = u]()->auto{
            return p->work(id);
        }));
        futures.emplace_back(pool.add_job([p = &D, id = u]()->auto{
            return p->work(id);
        }));
        futures.emplace_back(pool.add_job([p = &E, id = u]()->auto{
            return p->work(id);
        }));
        futures.emplace_back(pool.add_job([p = &F, id = u]()->auto{
            return p->work(id);
        }));
    }

    for (auto& f : futures) {
        temp.push_back(std::move(f.get()));
    }
    auto end = current_unix_epoch_time<std::chrono::nanoseconds>::get();
    for(const auto& v : temp){
        std::cout << v <<std::endl;
    }
    return end-start;
}

int main() {
    std::cout << std::fixed;
    std::cout << "=========================================================================" << std::endl;
    std::cout << "parallel(data1) "<< std::endl;
    std::size_t elapsed_time_parallel_nano1 = parallel_data_1_test();
    std::cout << "elapsed time(nanosecond) --> " << elapsed_time_parallel_nano1 << std::endl;
    std::cout << "=========================================================================" << std::endl;
    std::cout << "parallel(data1) "<< std::endl;
    std::size_t elapsed_time_parallel_nano2 = parallel_data_2_test();
    std::cout << "elapsed time(nanosecond) --> " << elapsed_time_parallel_nano2 << std::endl;
    std::cout << "=========================================================================" << std::endl;
    return 0;
}