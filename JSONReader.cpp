#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <utility>
using namespace std;
enum class TokenType {
    LeftCurly = '{',
    RightCurly = '}',
    LeftSquare = '[',
    RightSquare = ']',
    Colon = ':',
    Comma = ',',
    String
};
class Token {
   private:
    string _value;

   public:
    TokenType t;
    Token(TokenType t) : t(t) {}
    Token(TokenType t, string value) : t(t), _value(value) {}
    string getVal() { return _value; }
};
class ITokenizer {
   protected:
    queue<Token> result;

   public:
    ITokenizer(/* args */) {}
    virtual void tokenize(string json) { cout << "tokenize" << endl; }
    Token front() { return result.front(); }
    Token pop() {
        Token top = result.front();
        result.pop();
        return top;
    }
    Token back() { return result.back(); }
    int size() { return result.size(); }
    Token pop(TokenType type) {
        Token t = this->pop();
        if (t.t != type) {
            throw "Wrong Type";
        }
        return t;
    }
};
class QueryTokenizer : public ITokenizer {
   public:
    QueryTokenizer(/* args */) {}
    void tokenize(string json) {
        replaceAll(json, "\r\n", "\n");
        vector<string> lines = splitStr(json, "\n");
        for (auto& line : lines) {
            vector<string> queryItems = splitStr(line, ">");
            for (auto& queryItem : queryItems) {
                Token token(TokenType::String, queryItem);
                result.push(token);
            }
            result.push(Token(TokenType::Comma));
        }
    }
    void replaceAll(std::string& str, const std::string& from,
                    const std::string& to) {
        if (from.empty()) return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }
    vector<string> splitStr(string str, string delimiter) {
        vector<string> splited;
        string sub;
        size_t pos = 0;
        while ((pos = str.find(delimiter)) != string::npos) {
            sub = str.substr(0, pos);
            splited.push_back(sub);
            str.erase(0, pos + delimiter.length());
        }
        splited.push_back(str);
        return splited;
    }
};
class JsonTokenizer : public ITokenizer {
   private:
    enum class State { Neutral, Boolean, String, Number };
    bool _isNum(char c) { return (c <= '9' && c >= '0'); }
    bool _isNumStart(char c) { return _isNum(c) || c == '-'; }
    bool _isNumTail(char c) {
        return _isNum(c) || c == '.' || c == 'e' || c == 'E' || c == '+' ||
               c == '-';
    }
    bool _isBoolean(char c) {
        return c == 'f' || c == 'F' || c == 't' || c == 'T';
    }
    bool _isNull(char c) { return c == 'n' || c == 'N'; }
    void _pushToQueue(char c) {
        switch (c) {
            case '{':
                result.push(Token(TokenType::LeftCurly));
                break;
            case '}':
                result.push(Token(TokenType::RightCurly));
                break;
            case '[':
                result.push(Token(TokenType::LeftSquare));
                break;
            case ']':
                result.push(Token(TokenType::RightSquare));
                break;
            case ',':
                result.push(Token(TokenType::Comma));
                break;
            case ':':
                result.push(Token(TokenType::Colon));
                break;
            default:
                break;
        }
    }

   public:
    JsonTokenizer(/* args */) {}
    void tokenize(string json) {
        State state = State::Neutral;
        int layer = 0;
        string buffer;
        for (size_t i = 0; i < json.size(); i++) {
            char c = json[i];
            switch (state) {
                case JsonTokenizer::State::Neutral:
                    if (c == '\"') {
                        state = State::String;
                    } else if (_isNumStart(c)) {
                        buffer.push_back(c);
                        state = State::Number;
                    } else if (_isBoolean(c)) {
                        if (c == 'f' || c == 'F') {
                            Token f(TokenType::String, json.substr(i, 5));
                            result.push(f);
                            i += 4;
                        } else if (c == 't' || c == 'T') {
                            Token t(TokenType::String, json.substr(i, 4));
                            result.push(t);
                            i += 3;
                        }
                    } else if (_isNull(c)) {
                        Token f(TokenType::String, json.substr(i, 4));
                        result.push(f);
                        i += 3;
                    } else if (c == '}') {
                        result.push(Token(TokenType::RightCurly));
                        layer--;
                        if (layer == 0) return;
                        state = State::Neutral;
                    } else if (c == '{') {
                        layer++;
                        result.push(Token(TokenType::LeftCurly));
                    } else {
                        _pushToQueue(c);
                    }
                    break;
                case JsonTokenizer::State::String:
                    if (c == '\"') {
                        result.push(Token(TokenType::String, buffer));
                        buffer.clear();
                        state = State::Neutral;
                    } else if (c == '\\') {
                        if (json[i + 1] != '\"') {
                            buffer.push_back(c);
                        } else {
                            buffer.push_back(json[++i]);
                        }
                    } else {
                        buffer.push_back(c);
                    }
                    break;
                case JsonTokenizer::State::Number:
                    if (_isNumTail(c)) {
                        buffer.push_back(c);
                    } else {
                        result.push(Token(TokenType::String, buffer));
                        buffer.clear();
                        state = State::Neutral;
                        i--;
                    }
                    break;
                default:
                    break;
            }
        }
    }
};
enum class NodeType { ObjectNode, ListNode, LeafNode };
class Node {
   public:
    NodeType t;
    Node(NodeType nt) : t(nt) {}
};

class ObjectNode : public Node {
   public:
    vector<pair<string, Node*>> nodes;
    ObjectNode() : Node(NodeType::ObjectNode) {}
    ~ObjectNode() {}
    void addChild(string s, Node* n) {
        if (n == nullptr) {
            throw "nullptr";
        }
        nodes.push_back(make_pair(s, n));
    }
    bool hasKey(string s) {
        for (auto& x : nodes) {
            if (x.first == s) {
                return true;
            }
        }
        return false;
    }
    int getIndex(string s) {
        for (size_t i = 0; i < nodes.size(); i++) {
            if (nodes[i].first == s) {
                return i;
            }
        }
        return -1;
    }
};
class ListNode : public Node {
   public:
    vector<Node*> list;
    ListNode() : Node(NodeType::ListNode) {}
};
class LeafNode : public Node {
   private:
    string _value;

   public:
    LeafNode(string s) : _value(s), Node(NodeType::LeafNode) {}
    ~LeafNode() {}
    string getVal() { return _value; }
};
typedef vector<string> QueryObject;
class JsonParser {
   public:
    static Node* parse(ITokenizer& tokens) {
        Token token = tokens.pop();
        if (isLeafNode(token)) {
            return new LeafNode(token.getVal());
        }
        if (isListNode(token)) {
            ListNode* list = new ListNode();
            if (isEmptyList(tokens)) {
                tokens.pop(TokenType::RightSquare);
                return list;
            } else {
                while (true) {
                    Node* child = parse(tokens);
                    list->list.push_back(child);
                    token = tokens.pop();
                    if (token.t == TokenType::Comma &&
                        tokens.front().t == TokenType::RightSquare) {
                        tokens.pop(TokenType::RightSquare);
                        break;
                    }
                    if (token.t == TokenType::RightSquare) {
                        break;
                    }
                }
                return list;
            }
        }
        if (isObjectNode(token)) {
            ObjectNode* object = new ObjectNode();
            if (isEmptyObject(tokens)) {
                tokens.pop();
                return object;
            }
            while (true) {
                string token_key = tokens.pop(TokenType::String).getVal();
                token = tokens.pop(TokenType::Colon);
                Node* child = parse(tokens);
                object->addChild(token_key, child);
                token = tokens.pop();
                if (token.t == TokenType::Comma &&
                    tokens.front().t == TokenType::RightCurly) {
                    tokens.pop(TokenType::RightCurly);
                    break;
                }
                if (token.t == TokenType::RightCurly) break;
            }
            return object;
        }
        return NULL;
    }
    static bool isListNode(Token token) {
        return token.t == TokenType::LeftSquare;
    }
    static bool isObjectNode(Token token) {
        return token.t == TokenType::LeftCurly;
    }
    static bool isLeafNode(Token token) { return token.t == TokenType::String; }
    static bool isEmptyObject(ITokenizer tokens) {
        return tokens.front().t == TokenType::RightCurly;
    }
    static bool isEmptyList(ITokenizer tokens) {
        return tokens.front().t == TokenType::RightSquare;
    }
};
class QueryParser {
   public:
    static vector<QueryObject> parse(ITokenizer t) {
        vector<QueryObject> qs;
        QueryObject q;
        while (t.size() != 0) {
            if (t.front().t != TokenType::Comma) {
                q.push_back(t.pop().getVal());
            } else {
                qs.push_back(q);
                q.clear();
                t.pop();
            }
        }
        return qs;
    }
};
class QuerySolver {
   public:
    static int findFollowers(Node* j, QueryObject q, int idx) {
        if (idx == q.size()) {
            return printNode(j);
        }
        if (j->t == NodeType::LeafNode) {
            return 0;
        }
        if (j->t == NodeType::ListNode) {
            ListNode* root = (ListNode*)j;
            int sum = 0;
            for (auto x : root->list) {
                sum += findFollowers(x, q, idx);
            }
            return sum;
        }
        if (j->t == NodeType::ObjectNode) {
            ObjectNode* root = (ObjectNode*)j;
            int sum = 0;
            if (root->hasKey(q[idx])) {
                sum += findFollowers(root->nodes[root->getIndex(q[idx])].second,
                                     q, idx + 1);
            }
            return sum;
        }
    }
    static int findFirst(Node* j, QueryObject q) {
        if (j->t == NodeType::LeafNode) {
            return 0;
        }
        if (j->t == NodeType::ListNode) {
            ListNode* root = (ListNode*)j;
            int sum = 0;
            for (auto& x : root->list) {
                sum += findFirst(x, q);
            }
            return sum;
        }
        if (j->t == NodeType::ObjectNode) {
            ObjectNode* root = (ObjectNode*)j;
            int sum = 0;
            for (auto& x : root->nodes) {
                if (x.first == q.front()) {
                    sum += findFollowers(x.second, q, 1);
                }
                sum += findFirst(x.second, q);
            }
            return sum;
        }
        return 0;
    }
    static void solve(Node* j, QueryObject q) {
        ObjectNode* root = (ObjectNode*)j;
        string qkey = q.front();
        string jkey;
        int result = findFirst(j, q);
        if (result == 0) {
            cout << '\n';
        }
        return;
    }
    static int printNode(Node* n) {
        if (n->t == NodeType::LeafNode) {
            cout << ((LeafNode*)n)->getVal() << '\n';
            return 1;
        } else if (n->t == NodeType::ListNode) {
            ListNode* root = (ListNode*)n;
            int sum = 0;
            for (auto& x : root->list) {
                sum += printNode(x);
            }
            return sum;
        }
        return 0;
    }
};
class FileReader {
   private:
    static int _size(ifstream& file) {
        file.seekg(0, file.end);
        int size = file.tellg();
        file.seekg(0, file.beg);
        return size;
    }

   public:
    static string Read(ifstream& file) {
        int size = _size(file);
        char* buffer = new char[size + 1];
        file.read(buffer, size);
        buffer[size] = '\0';
        file.close();
        string json(buffer);
        delete[] buffer;
        return json;
    }
};
class JsonWTF {
   public:
    static string WTF(string s) {
        string str = "{\"Iotalh\":";
        if (s[0] == '[') {
            str += s;
            str += '}';
            return str;
        }
        return s;
    }
};

int main(int argc, char* argv[]) {
    ifstream fin1, fin2;
    fin1.open(argv[1], std::ifstream::binary);
    fin2.open(argv[2], std::ifstream::binary);
    JsonTokenizer j;
    QueryTokenizer q;
    j.tokenize(JsonWTF::WTF(FileReader::Read(fin1)));
    q.tokenize(FileReader::Read(fin2));
    Node* json = JsonParser::parse(j);
    bool isFirst = true;
    for (auto query : QueryParser::parse(q)) {
        if (isFirst) {
            isFirst = false;
        } else {
            cout << '\n';
        }
        QuerySolver::solve(json, query);
    }
}
