#ifndef LEXER_HPP
#define LEXER_HPP

#include<cstdio>
#include<cstdlib>
#include<fstream>
#include<list>
#include<string>
#include<vector>
#include "app.hpp"

// トークンの種類
// トークンが識別子、予約語、記号、数値のいずれかにあてはまるかという情報

/// トークン種別
enum TokenType {
    TOK_IDENTIFIER, // 識別子
    TOK_DIGIT,      // 数字
    TOK_SYMBOL,     // 記号
    TOK_INT,        // INT
    TOK_RETURN,     // RETURN
    TOK_EOF,        // EOF
};

/// 個別トークン格納クラス
class Token {
    private:
        TokenType Type;
        std::string TokenString;
        int Number;
        int Line;

    public:
    Token(std::string string, TokenType type, int line) : TokenString(string), Type(type), Line(line) {
        // 数字が入れられた場合
        if (type == TOK_DIGIT) {
            Number = atoi(string.c_str());
        } else {
            Number = 0x7fffffff;
        }
    };
    ~Token(){};

    // トークンの種別を取得
    TokenType getTokenType() { return Type; };

    // トークンの文字列表現を取得
    std::string getTokenString() { return TokenString; };

    // トークンの数値を取得
    int getNumberValue() { return Number; };

    // トークンの出現した行数を取得
    int getLine() { return Line; };
};

/// TokenStreamクラス
/// 切り出したすべてのTokenの格納と読み出し
class TokenStream {
    private:
      std::vector<Token*> Tokens;
      int CurIndex;

    public:
      TokenStream() : CurIndex(0){}
      ~TokenStream();

      bool ungetToken(int Times = 1);
      bool getNextToken();
      bool pushToken(Token *token) {
          Tokens.push_back(token);
          return true;
      }
      Token getToken();

      // トークンの種類を取得
      TokenType getCurType() {
          return Tokens[CurIndex] -> getTokenType();
      }

      // トークンの文字列表現を取得
      std::string getCurString() {
          return Tokens[CurIndex] -> getTokenString();
      }

      // トークンの数値を取得
      int getCurNumVal() {
          return Tokens[CurIndex] -> getNumberValue();
      }

      // 現在のインデックスを取得
      int getCurIndex() {
          return CurIndex;
      }

      // インデックスを指定した値に設定
      bool applyTokenIndex(int index) {
          CurIndex = index;
          return true;
      }
      bool printTokens();
};

TokenStream *LexicalAnalysis(std::string input_filename);

#endif