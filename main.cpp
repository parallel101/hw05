// С����ʦ��ҵ05����װ�Ƕ��߳� HTTP ������ - �����������Թپ��ú���
#include <functional>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <thread>
#include <map>
#include <shared_mutex>


struct User {
    std::string password;
    std::string school;
    std::string phone;
};

std::map<std::string, User> users;
std::map<std::string, std::chrono::steady_clock::time_point> has_login;  // ���� std::chrono::seconds ֮���
std::shared_mutex m_usertMtx;
std::shared_mutex m_loginMtx;

// ��ҵҪ��1������Щ������ɶ��̰߳�ȫ��
// ��ʾ������ȷ���� shared_mutex �ӷ֣��� lock_guard ϵ�мӷ�
std::string do_register(std::string username, std::string password, std::string school, std::string phone) {
    User user = {password, school, phone};
    std::unique_lock<std::shared_mutex> grd(m_usertMtx);
    bool ret = users.emplace(username, user).second;
    if (ret)
        return "ע��ɹ�";
    else
        return "�û����ѱ�ע��";
}

std::string do_login(std::string username, std::string password) {
    // ��ҵҪ��2���������¼��ʱ���ĳɻ��� chrono ��
    auto now = std::chrono::steady_clock::now();
    {
        std::shared_lock<std::shared_mutex> grd(m_loginMtx);
        if (has_login.find(username) != has_login.end()) {
            auto diff = now - has_login.at(username);
            int sec = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
            return std::to_string(sec) + "���ڵ�¼��";
        }
    }
    {
        std::unique_lock<std::shared_mutex> grd(m_loginMtx);
        has_login[username] = now;
    }

    std::shared_lock<std::shared_mutex> sl(m_usertMtx);
    if (users.find(username) == users.end())
        return "�û�������";
    if (users.at(username).password != password)
        return "�������";
    return "��¼�ɹ�";
}

std::string do_queryuser(std::string username) {
    std::shared_lock<std::shared_mutex> grd(m_usertMtx);
    if (users.find(username) == users.end())
        return "�û���������";

    auto& user = users.at(username);
    std::stringstream ss;
    ss << "�û���: " << username << std::endl;
    ss << "ѧУ:" << user.school << std::endl;
    ss << "�绰: " << user.phone << std::endl;
    return ss.str();
}


struct ThreadPool {
    void create(std::function<void()> start) {
        // ��ҵҪ��3�����������̱߳����ں�ִ̨�в�Ҫ�˳���
        // ��ʾ���ĳ� async �� future ���÷���ȷҲ���Լӷ�
        std::thread thr(start);
        m_pool.push_back(std::move(thr));
    }

    ~ThreadPool()
    {
        for (auto & thr : m_pool)
        {
            thr.join();
        }
    }

private:
    std::vector<std::thread> m_pool;
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
