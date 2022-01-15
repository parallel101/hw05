// С����ʦ��ҵ05����װ�Ƕ��߳� HTTP ������ - �����������Թپ��ú���
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>
#include <shared_mutex>
#include <chrono>

struct User {
    std::string password;
    std::string school;
    std::string phone;

};


std::shared_mutex m_mtx1;
std::shared_mutex m_mtx2;
std::shared_mutex m_mtx3;
std::shared_mutex m_mtx4;

std::map<std::string, User> users;
//std::map<std::string, long> has_login;  // ���� std::chrono::seconds ֮���
std::map<std::string, std::chrono::steady_clock::time_point> has_login;

// ��ҵҪ��1������Щ������ɶ��̰߳�ȫ��
// ��ʾ������ȷ���� shared_mutex �ӷ֣��� lock_guard ϵ�мӷ�
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    std::unique_lock grd(m_mtx1);
    User user = {password, school, phone};
    if (users.emplace(username, user).second)
        return "ע��ɹ�";
    else
        return "�û����ѱ�ע��";
}

std::string do_login(std::string username, std::string password) {
    // ��ҵҪ��2���������¼��ʱ���ĳɻ��� chrono ��
    std::shared_lock grd(m_mtx2);
    auto t0 = std::chrono::steady_clock::now();
    if(has_login.find(username)!=has_login.end()){
        auto dt = t0-has_login.at(username);
        int64_t sec = std::chrono::duration_cast<std::chrono::seconds>(dt).count();
        return std::to_string(sec) + "���ڵ�¼��";
    }
    {
        std::unique_lock grd(m_mtx4);
        has_login[username] = t0;
    }
    if (users.find(username) == users.end())
        return "�û�������";
    if (users.at(username).password != password)
        return "�������";
    return "��¼�ɹ�";

}

std::string do_queryuser(std::string username) {
    std::shared_lock grd(m_mtx3);
    auto &user = users.at(username);
    std::stringstream ss;
    ss << "�û���: " << username << std::endl;
    ss << "ѧУ:" << user.school << std::endl;
    ss << "�绰: " << user.phone << std::endl;
    return ss.str();
}


struct ThreadPool {
    std::vector<std::thread> m_pool;
public:
    void create(std::function<void()> start) {
        // ��ҵҪ��3�����������̱߳����ں�ִ̨�в�Ҫ�˳���
        // ��ʾ���ĳ� async �� future ���÷���ȷҲ���Լӷ�
        std::thread thr(start);
        m_pool.push_back(std::move(thr));
    }
    ~ThreadPool(){
        for(auto & t :m_pool) {
            t.join();
        }
    }

};

ThreadPool tpool;


namespace test {  // ������������ˮ������
std::string username[] = {"������", "������", "���ڱ�", "��ԭ��"};
std::string password[] = {"hellojob", "anti-job42", "cihou233", "reCihou_!"};
std::string school[] = {"�Űٰ�ʮ���Ь", "�㽭��Ь", "���Ŵ�Ь", "������ЬԺ"};
std::string phone[] = {"110", "119", "120", "12315"};
}

int main() {
    for (int i = 0; i < 262144; i++) {
        tpool.create([&] {
            std::cout << do_register(test::username[rand() % 4], test::password[rand() % 4], test::school[rand() % 4], test::phone[rand() % 4]) << std::endl;
        });
        tpool.create([&] {
            std::cout << do_login(test::username[rand() % 4], test::password[rand() % 4]) << std::endl;
        });
        tpool.create([&] {
            std::cout << do_queryuser(test::username[rand() % 4]) << std::endl;
        });
    }

    // ��ҵҪ��4���ȴ� tpool �������̶߳����������˳�

    return 0;
}
