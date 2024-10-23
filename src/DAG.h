//
// Created by jjangchan on 2024/10/17.
//

#ifndef VISUAL_CAMP_DAG_H
#define VISUAL_CAMP_DAG_H

#include <concepts>
#include <iostream>
#include <utility>
#include <vector>
#include <set>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <memory>

//=================================================== extern ===========================================================
#include "ThreadPool.h"
#include <boost/variant.hpp>
//======================================================================================================================

class Data;
class Node;

//=================================================== functor ==========================================================
// ---> square
template<typename T,
        typename = typename std::enable_if<std::is_same<T, double>::value>::type>
double get_value(Node& n, const T a){
    return n([](double x)->double{return x*x;},
             a);
}

// --> add
template<typename T,
        typename = typename std::enable_if<std::is_same<T, int>::value>::type>
int get_value(Node& n, T a, T b){
    return n([](int x, int y) -> int {return x+y;},
             a, b);
}

// ---> concat
template<typename T,
        typename = typename std::enable_if<std::is_same<T, std::string>::value>::type>
std::string get_value(Node& n, const T& a, const T& b){
    return n([](const std::string& a, const std::string& b)->std::string{return a+b;},
             a, b);
}

// ---> request
template<typename T,
        typename = typename std::enable_if<std::is_same<T, std::string>::value>::type>
std::unordered_map<std::string, std::string> get_value(Node& n, const T a){
    return n([](const std::string& url)->std::unordered_map<std::string, std::string>{return {};},
             a);
}

// ---> find max
template<typename A,
        typename = typename std::enable_if<std::is_same<A, std::vector<float>>::value>::type>
float get_value(Node& n, const A& a){
    return n([](const std::vector<float>& v) -> float{return *std::max_element(v.begin(), v.end());},
             a);
}
//======================================================================================================================


// --> functor data type
using send_value_type = boost::variant<int, double, float, std::string, std::vector<float>, std::unordered_map<std::string, std::string>>;


// --> uuid
struct uuid{
    static std::string generate_uuid(){
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        return boost::uuids::to_string(uuid);
    }
};


class Data : public std::enable_shared_from_this<Data>{
private:
    // --> data unique id
    std::string data_id;
    // --> functor data vector
    std::vector<send_value_type> in_data;
public:
    Data(std::vector<send_value_type> data, std::string id) : in_data(std::move(data)), data_id(std::move(id)){
        //std::cout << "(" << this << ") constructor" << std::endl;
    }
    ~Data(){
        //std::cout << "(" << this << ") destructor" << std::endl;
    }

public:
    std::vector<send_value_type>& operator()(){return in_data;}
    std::shared_ptr<Data> get_shared_ptr() { return shared_from_this(); }
    const std::string& get_id() const {return data_id;}

};

class Node{
private:
    class in_impl;
    class out_impl;

    // --> output
    // -> 자신에게 들어온 간선(input data들)을 functor 계산하고
    // -> 링크로 연결된 다음 노드(input)들에게 data 전달
    std::unique_ptr<out_impl> out_pimpl;
    // --> input
    // -> output으로 받은 데이터를 보관
    std::vector<std::unique_ptr<in_impl>> in_pimpl;

public:
    Node(): out_pimpl(std::make_unique<out_impl>(this)){}
    ~Node() = default;

    // concept regular_invocable = invocable<F, Args...> && copy_constructible<F>;
    // --> 호출 && 복사 가능 한지 컴파일 타임에서 검증
    template<typename F, typename... Args>
    requires std::regular_invocable<F, Args...>
    auto operator()(F f, Args&&... args){
        auto cal = f(std::forward<Args>(args)...);
        return cal;
    }


    /**
    int operator()(int a, int b){ return a+b;}
    std::string operator()(const std::string& a, std::string& b){ return a+b;}
    double operator()(double x){return x*x;}
    std::unordered_map<std::string, std::string> operator()(const std::string& url){return {};}
    float operator()(const std::vector<float>& v) {return *std::max_element(v.begin(), v.end());}
    **/

public:
    out_impl* get_out_port() const { return out_pimpl.get();}
    in_impl* get_in_port(const int index){
        if(index > in_pimpl.size()) assert(!"invalid input port size");
        if(in_pimpl.size() == index) in_pimpl.push_back(std::make_unique<in_impl>(this));
        return in_pimpl[index].get();
    }

    void make_edge(in_impl* in) const;
    std::vector<send_value_type> work(const std::string& id) const;
    std::vector<send_value_type> serial(const std::string& id) const;

};

class Node::in_impl{
private:
    // ---> 고유 data id를 보관하는 map
    // -> 들어온 data를 처리하기 위한 map
    // -> uuid로 데이터의 고유 id값을 키값으로 가진다.
    std::unordered_map<std::string, std::shared_ptr<Data>> m_data;
    Node* node;

public:
    in_impl(Node* n): node(n), m_data({}){}
    ~in_impl() = default;

    in_impl(in_impl&& in) noexcept = default;
    in_impl& operator=(in_impl&& in) noexcept = default;

    // !!! double delete 방지
    in_impl(const in_impl& in) = delete;
    in_impl& operator=(const in_impl& in) = delete;


public:

    // [ 해당 data 고유 id에 functor data가 들어 왔는지 확인하는 함수 ]
    bool empty(const std::string& id) {
        auto iter = m_data.find(id);
        if(m_data.find(id) == m_data.end()) return true;
        return false;
    }

    // [ output 에서 계산된 functor data를 담아주는 함수 ]
    void put_data(const std::string& id, std::shared_ptr<Data> d){ m_data[id] = std::move(d);}

    Data* get_data(const std::string& id) {
        return m_data[id].get();
    }

    void m_erase(const std::string& id){
        m_data.erase(id);
    }

    std::shared_ptr<Data>* get_address(const std::string& id) {
        return &m_data[id];
    }

    std::size_t data_use_count(const std::string& id) {
        if(empty(id)) return 0;
        return m_data[id].use_count();
    }

};


struct variant_visitor : public boost::static_visitor<int>{
    int operator() (int value) const {return (int)value;}
    int operator() (double value) const {return (int)value;}
    int operator() (float value) const {return (int)value;}
    int operator() (const std::string& s) const {return 0;}
    int operator() (const std::vector<float>& v) const {return 0;}
    int operator() (const std::unordered_map<std::string, std::string>& m) const {return 0;}
};

class Node::out_impl{
private:
    // --> output 으로 처리된 데이터를 연결된 노드들로 전달하는 컨테이너
    std::set<in_impl*> set_link;
    Node* node;
    // --> 더이상 output 없을 경우 보관
    std::shared_ptr<Data> non_out_put;
public:
    out_impl(Node* n) : node(n){}
    ~out_impl() = default;

    out_impl(out_impl&& out) noexcept = default;
    out_impl& operator=(out_impl&& out) noexcept = default;

    // !!! double delete 방지
    out_impl(const out_impl& out) = delete;
    out_impl& operator=(const out_impl& out) = delete;

public:
    bool end() const {return non_out_put != nullptr;}
    Data* get_data() const {return non_out_put.get();}
    void link(in_impl* _in){ set_link.insert(_in);};

    // [ input 으로 연결된 데이터 functor 계산 함수 ]
    // 1. 자신에게 들어올 데이터가 모두 오면 작업을 실시
    // 2. 데이터 타입에 맞게 functor 처리 후
    // 3. 데이터 (shared_ptr) 생성 후 set_link 에 연결된 다음 노드들에게 데이터 소유권 이동
    // 4. 작업이 끝나면 해당 노드 데이터(shared_ptr)를 비어주고 다음 작업을 기다린다.
    std::vector<send_value_type> send_data(const std::vector<std::unique_ptr<in_impl>>& datas, const std::string& id){
        //printf("in --> %p \n", node);
        if(datas.size() == 1){
            if(datas[0]->empty(id)){
                return {};
            }

            std::vector<send_value_type>& value = datas[0]->get_data(id)->operator()();
            // functor로 계산된 데이터를 보관하는 vector
            std::vector<send_value_type> cal_v(value.size());

            // 1개의 인자 functor 계산
            for(int i = 0; i < value.size(); i++){
                // --> square type 일 경우
                if(const int *p = boost::get<int>(&value[i]))  cal_v[i] = get_value(*node, (double)*p);
                else if(const double *p = boost::get<double>(&value[i]))  cal_v[i] = get_value(*node, *p);
                else if(const float *p = boost::get<float>(&value[i]))  cal_v[i] = get_value(*node, (double)*p);

                // --> find_max type 일 경우
                if(const auto *p = boost::get<std::vector<float>>(&value[i])) cal_v[i] = get_value(*node, *p);
                // --> request type 일 경우
                if(const auto *p = boost::get<std::string>(&value[i])) cal_v[i] = get_value(*node, *p);
            }

            // 링크된 다음 노드들 전달 준비
            if(!set_link.empty()){
                // 링크된 다음 노드(input)에게 계산된 vector(functor data) 전달
                for(in_impl* in : set_link){
                    //--> shared_ptr data instance 생성 후 functor 로 계산된 데이터 소유권 이동
                    std::shared_ptr<Data> new_data = std::make_shared<Data>(cal_v, id);
                    in->put_data(id, std::move(new_data));
                }
                //--> 다음 노드로 데이터는 전달 완료 되고,
                //--> 감싸진 데이터 instance 소유권은 연결된 노드로 이동 되어서,
                //--> 현재 input class 가 가지고 있는 데이터 instance 삭제
                datas[0]->get_address(id)->reset();
            }else {
                non_out_put = std::make_shared<Data>(cal_v, id);
                datas[0]->get_address(id)->reset();
            }

            //--> input class key로 매칭되는 map 삭제
            datas[0]->m_erase(id);
            // --> 현재 input class 에 감싸진 shred_ptr 소멸
            return cal_v;

        }else if(datas.size() > 1){
            if(datas[0]->empty(id) || datas[1]->empty(id)) {
                return {};
            }
            // 연결된 input data instance 소유권 이동
            std::shared_ptr<Data> d1 = std::move(*datas[0]->get_address(id));
            std::shared_ptr<Data> d2 = std::move(*datas[1]->get_address(id));

            // get vector(functor data)
            std::vector<send_value_type> & v1 = d1->operator()();
            std::vector<send_value_type> & v2 = d2->operator()();
            assert(v1.size() == v2.size());
            std::vector<send_value_type> cal_v(v1.size());

            // 2개의 인자 functor 계산
            for(int i = 0; i < v1.size(); i++){
                // --> concat 일 경우
                if(boost::get<std::string>(&v1[i]) && boost::get<std::string>(&v2[i])){
                    cal_v[i] = get_value(*node, *(boost::get<std::string>(&v1[i])), *(boost::get<std::string>(&v2[i])));
                }else{
                    // --> add 일 경우
                    int a = boost::apply_visitor(variant_visitor(), v1[i]);
                    int b = boost::apply_visitor(variant_visitor(), v2[i]);
                    cal_v[i] = get_value(*node, a, b);
                }
            }

            // 링크된 다음 노드들 전달 준비
            if(!set_link.empty()){
                for(in_impl* in : set_link){
                    //--> 연결된 링크에 data instance 생성 후 소유권 이동
                    std::shared_ptr<Data> new_data = std::make_shared<Data>(cal_v, id);
                    in->put_data(id, std::move(new_data));
                }
                //--> 인스턴스 삭제
                d1.reset();
            }else{
                non_out_put = std::make_shared<Data>(cal_v, id);
            }

            datas[0]->m_erase(id);
            datas[1]->m_erase(id);
            // --> d2, d1 소멸
            return cal_v;
        }
        return {};
    }

private:

};

std::ostream& operator<<(std::ostream& os, const std::vector<send_value_type>& v){
    os << "{";
    for(int i = 0; i < v.size(); i++){
        if(const auto *p = boost::get<int>(&v[i])) os << *p;
        if(const auto *p = boost::get<double>(&v[i])) os << *p;
        if(const auto *p = boost::get<float>(&v[i])) os << *p;
        if(const auto *p = boost::get<std::string>(&v[i])) os << *p;
        if(i < v.size()-1) os << ",";
    }
    os << "}";
    return os;
}

// [thread pool job(node) 등록 함수]
// 1. package_task 에 등록
// 2. promise 에 반환 값 또는 exception 객체 등록
// 3. future 에서 get_future()로 통해 반환값 획득
std::vector<send_value_type> Node::work(const std::string& id) const {
    // --> 해당 노드에게 들어오는 input data가 모두 들어오면 작업을 실시
    std::size_t wait = 0, run = in_pimpl.size();
    while(wait < run){
        wait = 0;
        for(const auto& p : in_pimpl) {
            if(!p->empty(id)) wait++;
        }
    }
    return out_pimpl->send_data(in_pimpl, id);
}

// [노드 연결 함수]
void Node::make_edge(in_impl *in) const {
    out_pimpl->link(in);
}

std::vector<send_value_type> Node::serial(const std::string &id) const {
    return out_pimpl->send_data(in_pimpl, id);
}

#endif //VISUAL_CAMP_DAG_H
