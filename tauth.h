#pragma once

#include "theader.h"

#include "json.hpp"
using json = nlohmann::json;

/*
Authentication

Сохранение пароля:
Генерируем длинную случайную соль, используя криптографически стойкий генератор псевдослучайных чисел;
Присоединяем соль к паролю и вычисляем хеш-код с помощью стандартной криптографической хеш-функции, например, SHA256;
Сохраняем и соль, и хеш-код в записи базы данных пользователей.

Проверка пароля:
Извлекаем соль и хеш-код пользователя из базы;
Добавляем соль к введенному паролю и вычисляем хеш-код с помощью той же самой функции;
Сравниваем хеш-код введенного пароля с хеш-кодом из базы данных. Если они совпадают, пароль верен. В противном случае, пароль введен неправильно.
*/


struct _info_auth {
    string session_login;
    TIPAddress session_ip;
    string session_token;
    time_t session_time_start;
    double session_length;
};



class TAuth
{
  struct _s_account {
    string login;
    string salt;
    string hash;
    double session_length;
    };
  vector <_s_account> accounts;

  std::mt19937 random_generator_;

  bool LoginAdd(bool admin,const string& login,const string& password,double session_length);
  void Write();

  bool LoginVerify(const string& login,const string& password);


public:
  TAuth();
  void Read();

  _info_auth Session_Login(const string& login,const string& password,const string& unique_id);   //return token session
};

extern TAuth Auth;
