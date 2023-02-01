#include "tauth.h"

#include "sha256.h"
#include "tlog.h"



TAuth        Auth;

TAuth::TAuth()
{
    std::random_device device;
    random_generator_.seed(device());
}

void TAuth::Read()
{
  json j;

   ifstream acc_file ("./accounts.cfg");
   if (acc_file.is_open())
   {
       try {
          j = json::parse(acc_file);
          }
       catch(...) {
          Log.ScrFile("bad file accounts.cfg\n");
          }
    acc_file.close();
    chmod("./accounts.cfg",0666);

    unsigned idx=0;

 try {
    for (auto i : j["accounts"]) {
       _s_account tmp;
       tmp.login=j["accounts"].at(idx);
       tmp.salt=j[tmp.login].at(0);
       tmp.hash=j[tmp.login].at(1);
       tmp.session_length=j[tmp.login].at(2);
       accounts.push_back(tmp);
       ++idx;
      }
     }
 catch(...) {
    Log.Scr("bad format accounts.cfg, create default file\n");
    LoginAdd(true,"admin","admin",60.0*60);   // 60 min
    LoginAdd(true,"user","user",60.0*60);    // 60 min
    return;
    }
    Log.Scr("read accounts.cfg ok\n");
   }

   else {
     Log.Scr("unable to open accounts.cfg, create default file\n");
     LoginAdd(true,"admin","admin",60.0*60);   // 60 min
     LoginAdd(true,"user","user",60.0*60);    // 60 min
     }


}

void TAuth::Write()
{
  json j;

  for (auto & akkount : accounts)   {
       j[akkount.login] += akkount.salt;
       j[akkount.login] += akkount.hash;
       j[akkount.login] += akkount.session_length;
       j["accounts"]    += akkount.login;
      }

  ofstream acc_file ("./accounts.cfg");
  if (acc_file.is_open())
     acc_file << j.dump();
   acc_file.close();
   chmod("./accounts.cfg",0666);
}



bool TAuth::LoginAdd(bool admin,const string& login,const string& password,double session_length)
{
  if (!admin) return false;

  for (auto & akkount : accounts)   {
      if (akkount.login==login)
              return false;
      }

  string f_rnd = std::to_string(random_generator_());
  string f_ixash = sha256(f_rnd);
  struct _s_account passw;
  passw.login = login;
  passw.salt = f_ixash.substr(0,22);        // генерируем случайную строку длиной в 22 символа
  passw.hash = sha256(passw.salt+password); // формируем хеш пароля
  passw.session_length=session_length;
  accounts.push_back(passw);
  Write();
  return true;
}


bool TAuth::LoginVerify(const string& login,const string& password)
{
  for (unsigned int i=0;i<accounts.size();i++) {
      if (accounts[i].login==login) {
          string hash = sha256(accounts[i].salt+password); // формируем хеш пароля
          return (hash==accounts[i].hash);
          }
      }
  return false;
}


//********************************************************************************************
//    SESSIONs
//********************************************************************************************

//session_length - длительность сессии в секундах
_info_auth TAuth::Session_Login(const string& login,const string& password,const string& unique_id)
{
    _info_auth info {};

  if (!LoginVerify(login,password)) return info;

  _s_account *p=&accounts[0];

  for(unsigned int i=0;i<accounts.size();++i) {

      if (p->login==login) {
        info.session_time_start=std::time(nullptr);
        info.session_length = accounts[i].session_length;
        string s_rnd = std::to_string(random_generator_());
        string token = sha256(unique_id+s_rnd);
        info.session_token=token;
        return info;
        }
      ++p;
      }

// empty
return info;
}


