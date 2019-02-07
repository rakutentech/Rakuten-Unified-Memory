/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   base64.h
 * 
 * This was taken from https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
 *
 * Created on 2018/10/19, 12:22
 */

#ifndef BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A
#define BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A

#include <string>

    
//
//  base64 encoding and decoding with C++.
//  Version: 1.01.00
//
using namespace std;

class base64 {
public:
    base64();
    base64(const base64& orig);
    virtual ~base64();

    static string encode(unsigned char const* bytse_to_encode, unsigned int in_len);
    static string decode(string const& encoded_string);
private:
};
#endif /* BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A */

