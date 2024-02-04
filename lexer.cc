/*
 * Copyright (C) Rida Bazzi
 *
 * Do not share this file with anyone
 */
#include <iostream>
#include <istream>
#include <vector>
#include <string>
#include <cctype>

#include "lexer.h"
#include "inputbuf.h"

using namespace std;

string reserved[] = { "END_OF_FILE",
    "IF", "WHILE", "DO", "THEN", "PRINT",
    "PLUS", "MINUS", "DIV", "MULT",
    "EQUAL", "COLON", "COMMA", "SEMICOLON",
    "LBRAC", "RBRAC", "LPAREN", "RPAREN",
    "NOTEQUAL", "GREATER", "LESS", "LTEQ", "GTEQ",
    "DOT", "NUM", "ID", "ERROR", "REALNUM", "BASE08NUM", "BASE16NUM" // TODO: Add labels for new token types here (as string)
};

#define KEYWORDS_COUNT 5
string keyword[] = { "IF", "WHILE", "DO", "THEN", "PRINT" };

void Token::Print()
{
    cout << "{" << this->lexeme << " , "
         << reserved[(int) this->token_type] << " , "
         << this->line_no << "}\n";
}

LexicalAnalyzer::LexicalAnalyzer()
{
    this->line_no = 1;
    tmp.lexeme = "";
    tmp.line_no = 1;
    tmp.token_type = ERROR;
}

bool LexicalAnalyzer::SkipSpace()
{
    char c;
    bool space_encountered = false;

    input.GetChar(c);
    line_no += (c == '\n');

    while (!input.EndOfInput() && isspace(c)) {
        space_encountered = true;
        input.GetChar(c);
        line_no += (c == '\n');
    }

    if (!input.EndOfInput()) {
        input.UngetChar(c);
    }
    return space_encountered;
}

bool LexicalAnalyzer::IsKeyword(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (s == keyword[i]) {
            return true;
        }
    }
    return false;
}

bool LexicalAnalyzer::IsBase8Digit(char c)
{
    return (c >= '0' && c <= '7');
};

bool LexicalAnalyzer::IsBase16Digit(char c)
{
    // return ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'));
    return ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'));

};

TokenType LexicalAnalyzer::FindKeywordIndex(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (s == keyword[i]) {
            return (TokenType) (i + 1);
        }
    }
    return ERROR;
}

Token LexicalAnalyzer::ScanNumber()
{
    char c;
    int b16Count = 0;
    // char b16Char;
    input.GetChar(c);
    if (isdigit(c)) {
        if (!input.EndOfInput() && c == '0') {      //number starting with '0'
            tmp.lexeme = "0";
        } else {                                    //number not starting with '0' but (1...9)
            tmp.lexeme = "";
            while (!input.EndOfInput() && isdigit(c)) {         //append all 0...9 digits
                tmp.lexeme += c;
                input.GetChar(c);
            }
            while(!input.EndOfInput() && IsBase16Digit(c)){     //append all B16 digits
                b16Count++;
                tmp.lexeme += c;
                input.GetChar(c);
            }

            //starting analyze a BASE16
            if(!input.EndOfInput() && c == 'x'){        //c is a 'x'. This tokentype might be B16
                tmp.lexeme += 'x';
                input.GetChar(c);                
                if (!input.EndOfInput() && c == '1'){   //c is a 1 --> tokentype might be B16
                    tmp.lexeme += '1';
                    input.GetChar(c);
                    if(c == '6'){                       //c is 6 --> B16 recognized
                        tmp.lexeme += '6';
                        tmp.token_type = BASE16NUM;
                        tmp.line_no = line_no;
                        return tmp;           
                    } else {                            //c is not 6;
                        if(!tmp.lexeme.empty()){
                            tmp.lexeme.pop_back();      //pop 1
                            tmp.lexeme.pop_back();      //pop x

                        }
                        input.UngetChar(c);
                        input.UngetChar('1');
                        input.UngetChar('x');           
                        while (!input.EndOfInput() && b16Count > 0) {
                        if(!tmp.lexeme.empty()){
                            c = tmp.lexeme.back(); 
                            tmp.lexeme.pop_back();      //remove B16digits from lexeme
                        }
                        input.UngetChar(c);
                        b16Count--;
                    }
                    }
                } else {
                    if(!tmp.lexeme.empty()){
                        tmp.lexeme.pop_back();          //pop x
                    }
                    input.UngetChar(c); 
                    input.UngetChar('x');
                    while (!input.EndOfInput() && b16Count > 0) {
                        if(!tmp.lexeme.empty()){
                            c = tmp.lexeme.back(); 
                            tmp.lexeme.pop_back();      //remove B16digits from lexeme
                        }
                        input.UngetChar(c);
                        b16Count--;
                    }
                }
            } else {                                //c is not an 'x'; return c to the buffer                
                input.UngetChar(c);
                while (!input.EndOfInput() && b16Count > 0) {
                    if(!tmp.lexeme.empty()){
                        c = tmp.lexeme.back(); 
                        tmp.lexeme.pop_back();      //remove the 'dot' from lexeme
                    }
                    input.UngetChar(c);
                    b16Count--;
                }
                // cout << "After analyzing: " << tmp.lexeme << "\n";                
            }
            //ending analyze a BASE16                     
        }

        input.GetChar(c);
        //starting analyze a REALNUM
        if(!input.EndOfInput() && c == '.'){                           //read a dot
            tmp.lexeme += '.';
            input.GetChar(c);
            while (!input.EndOfInput() && isdigit(c)) {
                tmp.lexeme += c;                //append all the digits after the dot
                input.GetChar(c);
            }
            if(!isdigit(tmp.lexeme.back())){    //if last char is not a digit --> pop_back the dot
                if(!tmp.lexeme.empty()){
                    tmp.lexeme.pop_back();      //remove the 'dot' from lexeme
                }
                input.UngetChar(c);
                input.UngetChar('.');           // jump to tmp.token_type = NUM                    
            } else {                            //last char is a digit
                if (!input.EndOfInput()) {
                    input.UngetChar(c);
                }
                tmp.token_type = REALNUM;
                tmp.line_no = line_no;
                return tmp;
            }
        }
        //ending analyze a REALNUM
        
        //starting analyze a BASE08
        else if(!input.EndOfInput() && c == 'x'){     //c is not a dot; c is an 'x' might be BASE08; BASE16                      //c is a 'x'. This tokentype might be B8 or B16
            tmp.lexeme += 'x';
            input.GetChar(c);
            if(!input.EndOfInput() && c == '0'){                       //c is a 0
                tmp.lexeme += '0';
                input.GetChar(c);
                if(c == '8'){                   //c is a 8 --> BASE08
                    tmp.lexeme += '8';
                    tmp.token_type = BASE08NUM;
                    tmp.line_no = line_no;
                    return tmp;                        
                } else {                        //c is not a 8
                    if(!tmp.lexeme.empty()){
                        tmp.lexeme.pop_back();  //pop 0
                        tmp.lexeme.pop_back();  //pop x
                    }
                    input.UngetChar(c);
                    input.UngetChar('0');
                    input.UngetChar('x'); // jump to tmp.token_type = NUM
                }
        //ending analyze a BASE08

            //starting analyze a BASE16                
            } else if (!input.EndOfInput() && c == '1'){                            //c is a 1 --> tokentype might be B16
                tmp.lexeme += '1';
                input.GetChar(c);
                if(c == '6'){                       //c is 6 --> B16 recognized
                    tmp.lexeme += '6';
                    tmp.token_type = BASE16NUM;
                    tmp.line_no = line_no;
                    return tmp;           
                } else {                            //c is not 6;
                    if(!tmp.lexeme.empty()){
                        tmp.lexeme.pop_back();      //pop 1
                        tmp.lexeme.pop_back();      //pop x

                    }
                    input.UngetChar(c);
                    input.UngetChar('1');
                    input.UngetChar('x'); // jump to tmp.token_type = NUM
                }
            //ending analyze a BASE16

            } else {
                if(!tmp.lexeme.empty()){
                        tmp.lexeme.pop_back();
                    }
                    input.UngetChar(c); // jump to tmp.token_type = NUM
                    input.UngetChar('x');
            }

        } else {                                      //c is not an 'x' or a dot; return c to the buffer
            if (!input.EndOfInput()) {
                input.UngetChar(c);
            }
        }                  
        // TODO: You can check for REALNUM, BASE08NUM and BASE16NUM here!        
        tmp.token_type = NUM;
        tmp.line_no = line_no;
        return tmp;
    } else {
        if (!input.EndOfInput()) {
            input.UngetChar(c);
        }
        tmp.lexeme = "";
        tmp.token_type = ERROR;
        tmp.line_no = line_no;
        return tmp;
    }
}

Token LexicalAnalyzer::ScanIdOrKeyword()
{
    char c;
    input.GetChar(c);

    if (isalpha(c)) {
        tmp.lexeme = "";
        while (!input.EndOfInput() && isalnum(c)) {
            tmp.lexeme += c;
            input.GetChar(c);
        }
        if (!input.EndOfInput()) {
            input.UngetChar(c);
        }
        tmp.line_no = line_no;
        if (IsKeyword(tmp.lexeme))
            tmp.token_type = FindKeywordIndex(tmp.lexeme);
        else
            tmp.token_type = ID;
    } else {
        if (!input.EndOfInput()) {
            input.UngetChar(c);
        }
        tmp.lexeme = "";
        tmp.token_type = ERROR;
    }
    return tmp;
}

// you should unget tokens in the reverse order in which they
// are obtained. If you execute
//
//    t1 = lexer.GetToken();
//    t2 = lexer.GetToken();
//    t3 = lexer.GetToken();
//
// in this order, you should execute
//
//    lexer.UngetToken(t3);
//    lexer.UngetToken(t2);
//    lexer.UngetToken(t1);
//
// if you want to unget all three tokens. Note that it does not
// make sense to unget t1 without first ungetting t2 and t3
//
TokenType LexicalAnalyzer::UngetToken(Token tok)
{
    tokens.push_back(tok);;
    return tok.token_type;
}

Token LexicalAnalyzer::GetToken()
{
    char c;

    // if there are tokens that were previously
    // stored due to UngetToken(), pop a token and
    // return it without reading from input
    if (!tokens.empty()) {
        tmp = tokens.back();
        tokens.pop_back();
        return tmp;
    }

    SkipSpace();
    tmp.lexeme = "";
    tmp.line_no = line_no;
    input.GetChar(c);
    switch (c) {
        case '.':
            tmp.token_type = DOT;
            return tmp;
        case '+':
            tmp.token_type = PLUS;
            return tmp;
        case '-':
            tmp.token_type = MINUS;
            return tmp;
        case '/':
            tmp.token_type = DIV;
            return tmp;
        case '*':
            tmp.token_type = MULT;
            return tmp;
        case '=':
            tmp.token_type = EQUAL;
            return tmp;
        case ':':
            tmp.token_type = COLON;
            return tmp;
        case ',':
            tmp.token_type = COMMA;
            return tmp;
        case ';':
            tmp.token_type = SEMICOLON;
            return tmp;
        case '[':
            tmp.token_type = LBRAC;
            return tmp;
        case ']':
            tmp.token_type = RBRAC;
            return tmp;
        case '(':
            tmp.token_type = LPAREN;
            return tmp;
        case ')':
            tmp.token_type = RPAREN;
            return tmp;
        case '<':
            input.GetChar(c);
            if (c == '=') {
                tmp.token_type = LTEQ;
            } else if (c == '>') {
                tmp.token_type = NOTEQUAL;
            } else {
                if (!input.EndOfInput()) {
                    input.UngetChar(c);
                }
                tmp.token_type = LESS;
            }
            return tmp;
        case '>':
            input.GetChar(c);
            if (c == '=') {
                tmp.token_type = GTEQ;
            } else {
                if (!input.EndOfInput()) {
                    input.UngetChar(c);
                }
                tmp.token_type = GREATER;
            }
            return tmp;
        default:
            if (isdigit(c)) {
                input.UngetChar(c);
                return ScanNumber();
            } else if (isalpha(c)) {
                input.UngetChar(c);
                return ScanIdOrKeyword();
            } else if (input.EndOfInput())
                tmp.token_type = END_OF_FILE;
            else
                tmp.token_type = ERROR;

            return tmp;
    }
}

int main()
{
    LexicalAnalyzer lexer;
    Token token;

    token = lexer.GetToken();
    token.Print();
    while (token.token_type != END_OF_FILE)
    {
        token = lexer.GetToken();
        token.Print();
    }
}
