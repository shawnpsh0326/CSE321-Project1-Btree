#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <algorithm>

using namespace std;

struct Student {
    int id;
    string name;
    string gender;
    double gpa;
    double height;
    double weight;
};

struct Node {
    bool isLeaf;
    vector<int> keys;
    vector<int> rids;
    vector<Node*> children;
    Node* next; // leaf에서 다음 leaf 가리키는 포인터

    Node(bool leaf) {
        isLeaf = leaf;
        next = nullptr;
    }
};

class BPlusTree {
private:
    Node* root;
    int d;
    int splitNum;

    int maxKeys() const {return d - 1;}

    //int minKeys() const {return (d + 1) / 2 - 1;}
    int minLeafKeys() const {return d / 2;}
    int minInternalKeys() const {return (d + 1) / 2 - 1;}

    int maxChildrenInternal() const {return d;}
    int minChildrenInternal() const {return (d + 1) / 2;}

    int search(Node* node, int key) {
        // 먼저 leaf까지 내려가기
        while (!node->isLeaf) {
            int index = 0;

            while (index < node->keys.size() && key >= node->keys[index]) {
                index++;
            }

            node = node->children[index];
        }

        int index = 0;

        while (index < node->keys.size() && node->keys[index] < key) {
            index++;
        }

        // leaf에 있으면 반환
        if (index < node->keys.size() && node->keys[index] == key) 
            return node->rids[index];

        // 없으면 실패
        return -1;
    }

    void insert(Node* node, int key, int rid) {
        vector<Node*> path;
        vector<int> pathIndex;


        // 해당 key를 배치할 leaf까지 내려가기
        while (!node->isLeaf) {
            int index = 0;

            while (index < node->keys.size() && key >= node->keys[index]) {
                index++;
            }

            path.push_back(node);
            pathIndex.push_back(index);
            node = node->children[index];
        }

        // leaf에 key 삽입
        int position = 0;

        while (position < node->keys.size() && node->keys[position] < key) {
            position++;
        }

        // 만약 key에 해당하는 값이 이미 leaf에 있으면 rid 갱신
        if (position < node->keys.size() && node->keys[position] == key) {
            node->rids[position] = rid;
            return;
        }

        // leaf에 키 삽입
        node->keys.insert(node->keys.begin() + position, key);
        node->rids.insert(node->rids.begin() + position, rid);


        // 최대 key수를 초과하면 path를 따라 올라가면서 필요한만큼 split
        while (node->keys.size() > maxKeys()) {
            splitNum++;

            // 오른쪽에 새로운 node 만들기
            Node* right = new Node(node->isLeaf);

            int midKey;

            // leaf일 경우 - 올라가는 key가 그대로 leaf안에 있음
            if (node->isLeaf) {
                int mid = (node->keys.size() + 1) / 2;

                for (int i = mid; i < node->keys.size(); i++) {
                    right->keys.push_back(node->keys[i]);
                    right->rids.push_back(node->rids[i]);
                }
                node->keys.resize(mid);
                node->rids.resize(mid);

                // leaf 연결
                right->next = node->next;
                node->next = right;

                midKey = right->keys[0];
            }
            // internal node일 경우 - 올라가는 key는 위로 올리고 없어짐.
            else {
                int mid = node->keys.size() / 2;
                midKey = node->keys[mid];

                for (int i = mid + 1; i < node->keys.size(); i++) {
                    right->keys.push_back(node->keys[i]);
                }

                for (int i = mid + 1; i < node->children.size(); i++) {
                    right->children.push_back(node->children[i]);
                }

                node->keys.resize(mid);
                node->children.resize(mid + 1);
            }

            // root를 split해야 하는 경우
            if (path.empty()) {
                Node* newRoot = new Node(false);
                newRoot->keys.push_back(midKey);
                newRoot->children.push_back(node);
                newRoot->children.push_back(right);

                root = newRoot;
                return;
            }

            // parent에 mid 삽입
            Node* parent = path.back();
            int index = pathIndex.back();

            path.pop_back();
            pathIndex.pop_back();

            parent->keys.insert(parent->keys.begin() + index, midKey);
            parent->children.insert(parent->children.begin() + index + 1, right);

            // path의 위쪽 parent로 넘어가기
            node = parent;
        }


    }

    // delete 할 때 underflow가 나는지 확인
    bool underflow(Node* node) {
        if (node == root)
            return false;

        if (node->isLeaf)
            return node->keys.size() < minLeafKeys();

        return node->keys.size() < minInternalKeys();
    }

    // underflow가 발생했을 때 옆에서 가져올 수 있는지 확인
    bool borrow(Node* node) {
        if (node->isLeaf) 
            return node->keys.size() > minLeafKeys();
        
        return node->keys.size() > minInternalKeys();
    }

    // delete 할 때 형제에게서 가져올 수 없다면 node 합치기
    void merge(Node* parent, int index) {
        Node* left = parent->children[index];
        Node* right = parent->children[index + 1];

        // leaf일 때는 key랑 rid 합치기
        if (left->isLeaf) {
            for (int i = 0; i < right->keys.size(); i++) {
                left->keys.push_back(right->keys[i]);
                left->rids.push_back(right->rids[i]);
            }

            left->next = right->next;

            parent->keys.erase(parent->keys.begin() + index);
            parent->children.erase(parent->children.begin() + index + 1);

            delete right;
        }

        // internal일 때는 key랑 children 합치기
        else {
            left->keys.push_back(parent->keys[index]);

            for (int i = 0; i < right->keys.size(); i++) {
                left->keys.push_back(right->keys[i]);
            }

            for (int i = 0; i < right->children.size(); i++) {
                left->children.push_back(right->children[i]);
            }

            parent->keys.erase(parent->keys.begin() + index);
            parent->children.erase(parent->children.begin() + index + 1);

            delete right;
        }
    }

    // delete 할 때 underflow 났다면 고치기
    void fixUnderflow(Node* parent, int index) {
        Node* child = parent->children[index];

        // underflow가 아니라면 넘어가기
        if (!underflow(child))
            return;

        int l = 0;

        // 왼쪽 node로 고쳐보기
        if (index > 0) {
            Node* leftSib = parent->children[index - 1];

            // 왼쪽 node에서 하나 가져와도 문제 없다면 하나 가져오기
            if (borrow(leftSib)) {
                // leaf라면 key와 rid 옮기기
                if (child->isLeaf) {
                    child->keys.insert(child->keys.begin(), leftSib->keys.back());
                    child->rids.insert(child->rids.begin(), leftSib->rids.back());

                    leftSib->keys.pop_back();
                    leftSib->rids.pop_back();

                    parent->keys[index - 1] = child->keys[0];
                }

                // internal이라면 key와 children 옮기기
                else {
                    child->keys.insert(child->keys.begin(), parent->keys[index - 1]);
                    parent->keys[index - 1] = leftSib->keys.back();
                    leftSib->keys.pop_back();

                    child->children.insert(child->children.begin(), leftSib->children.back());
                    leftSib->children.pop_back();
                }

                return;
            }

            l = 1;
        }
            
        // 왼쪽 node가 없을 때 오른쪽 node로 고쳐보기
        if (index + 1 < parent->children.size()) {
            Node* rightSib = parent->children[index + 1];

            // 오른쪽 node에서 하나 가져와도 문제 없다면 가져오기
            if (borrow(rightSib)) {
                if (child->isLeaf) {
                    child->keys.push_back(rightSib->keys.front());
                    child->rids.push_back(rightSib->rids.front());

                    rightSib->keys.erase(rightSib->keys.begin());
                    rightSib->rids.erase(rightSib->rids.begin());

                    parent->keys[index] = rightSib->keys[0];
                } 
                else {
                    child->keys.push_back(parent->keys[index]);

                    parent->keys[index] = rightSib->keys.front();
                    rightSib->keys.erase(rightSib->keys.begin());

                    child->children.push_back(rightSib->children.front());
                    rightSib->children.erase(rightSib->children.begin());
                }

                return;
            }
        }

        // 만약 양쪽 모두 가져올 수 없다면 왼쪽이나 오른쪽과 merge하기
        if (l == 1)
            merge(parent, index - 1);
        else 
            merge(parent, index);
    }

    // 실제 delete하는 함수
    void remove(Node* node, int key) {
        int index = 0;

        // leaf라면 그냥 삭제
        if (node->isLeaf) {
            while (index < node->keys.size() && node->keys[index] < key) {
                index++;
            }

            if (index < node->keys.size() && node->keys[index] == key) {
                node->keys.erase(node->keys.begin() + index);
                node->rids.erase(node->rids.begin() + index);
            }

            return;
        }

        // leaf가 아니면 다음 child로 내려감
        while (index < node->keys.size() && key >= node->keys[index]) {
            index++;
        }

        Node* child = node->children[index];

        remove(child, key);

        // underflow 확인 및 수정
        if (index < node->children.size() && underflow(node->children[index])) {
            fixUnderflow(node, index);
        }

        // parent의 separator key 올바른지 확인 및 갱신
        if (index > 0 && index < node->children.size()) {
            Node* currentChild = node->children[index];

            if (!currentChild->keys.empty()) {
                node->keys[index - 1] = currentChild->keys[0];
            }
        }
    }

    // node utilization
    void utilization(Node* node, int& nodeNum, int& keyNum) {
        if (node == nullptr)
            return;
        
        nodeNum++;
        keyNum += node->keys.size();

        for (int i = 0; i < node->children.size(); i++)
            utilization(node->children[i], nodeNum, keyNum);
    }

public:
    BPlusTree(int order) {
        d = order;
        root = new Node(true);
        splitNum = 0;
    }

    int search(int key) {return search(root, key);}

    void insert(int key, int rid) {insert(root, key, rid);}

    void remove(int key) {
        remove(root, key);

        // root가 빈 경우
        if (!root->isLeaf && root->keys.size() == 0) {
            Node* oldRoot = root;
            root = root->children[0];
            delete oldRoot;
        }
    }

    // range query하는 함수
    vector<int> rangeQuery(int startKey, int endKey) {
        vector<int> result;

        Node* node = root;

        while (!node->isLeaf) {
            int index = 0;

            while (index < node->keys.size() && startKey >= node->keys[index]) {
                index++;
            }

            node = node->children[index];
        }

        while (node != nullptr) {
            for (int i = 0; i < node->keys.size(); i++) {
                if (node->keys[i] > endKey)
                    return result;

                if (node->keys[i] >= startKey)
                    result.push_back(node->rids[i]);
            }

            node = node->next;
        }

        return result;
    }

    // node utilization 계산
    double utilization() {
        int nodeNum = 0;
        int keyNum = 0;

        utilization(root, nodeNum, keyNum);

        if (nodeNum == 0)
            return 0;
        
        return (double)keyNum / (nodeNum * maxKeys());
    }

    int getSplitNum() {return splitNum;}
};

vector<Student> loadCSV(const string& filename) {
    vector<Student> students;

    ifstream file(filename);

    string line;

    //첫 줄인 header 버리기
    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string value;
        Student s;

        getline(ss, value, ',');
        s.id = stoi(value);

        getline(ss, s.name, ',');

        getline(ss, s.gender, ',');

        getline(ss, value, ',');
        s.gpa = stod(value);

        getline(ss, value, ',');
        s.height = stod(value);

        getline(ss, value, ',');
        s.weight = stod(value);

        students.push_back(s);
    }

    file.close();
    return students;
}

int main() {
    vector<Student> students = loadCSV("student.csv");

    bool exit = false;

    while (!exit) {
        bool order = false;
        int d;

        while (!order) {
            cout << "Input order(one integer): " << " ";
            cin >> d;
            if (d < 3) {
                cout << "Order must be at least 3." << endl << endl;
            }
            else
                order = true;
        }

        cout << endl;

        BPlusTree b_plus_tree(d);

        // insert 시간 측정
        auto insertStart = chrono::high_resolution_clock::now();
        for (int i = 0; i < students.size(); i++) {
            b_plus_tree.insert(students[i].id, i);
        }
        auto insertEnd = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> insertElapsed = insertEnd - insertStart;

        // search test
        vector<int> indices(students.size());
        for (int i = 0; i < students.size(); i++)
            indices[i] = i;

        random_device rd;
        mt19937 gen(rd());
        shuffle(indices.begin(), indices.end(), gen);
        
        int queryNum = 10000;
        int foundNum = 0;

        auto searchStart = chrono::high_resolution_clock::now();
        for (int i = 0; i < queryNum; i++) {
            int key = students[indices[i]].id;
            int rid = b_plus_tree.search(key);

            if (rid != -1)
                foundNum++;
        }
        auto searchEnd = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> searchElapsed = searchEnd - searchStart;

        // range query test
        int startKey = 202000000;
        int endKey = 202500000;

        double gpaSum = 0;
        double heightSum = 0;
        int maleNum = 0;

        auto rangeStart = chrono::high_resolution_clock::now();
        vector<int> rangeRids = b_plus_tree.rangeQuery(startKey, endKey);
        for (int rid : rangeRids) {
            if (students[rid].gender == "Male") {
                gpaSum += students[rid].gpa;
                heightSum += students[rid].height;
                maleNum++;
            }
        }
        auto rangeEnd = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> rangeElapsed = rangeEnd - rangeStart;

        // deletion test
        shuffle(indices.begin(), indices.end(), gen);
        int deleteNum = 2000;
        int successDeleteNum = 0;

        auto deleteStart = chrono::high_resolution_clock::now();
        for (int i = 0; i < deleteNum; i++) {
            int key = students[indices[i]].id;

            b_plus_tree.remove(key);

            if (b_plus_tree.search(key) == -1)
                successDeleteNum++;
        }
        auto deleteEnd = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> deleteElapsed = deleteEnd - deleteStart;

        // test 결과 출력
        cout << "Insertion time: " << insertElapsed.count() << " ms" << endl;
        cout << "Node utilization: " << b_plus_tree.utilization() * 100 << "%" << endl;
        cout << "Number of splits: " << b_plus_tree.getSplitNum() << endl << endl;

        cout << "Search queries: " << queryNum << endl;
        cout << "Found number: " << foundNum << endl;
        cout << "Total search time: " << searchElapsed.count() << " ms" << endl;
        cout << "Mean search time: " << searchElapsed.count() / queryNum << " ms" << endl << endl;;

        cout << "Range query: " << startKey << " to " << endKey << endl;
        cout << "Range result count: " << rangeRids.size() << endl;
        cout << "Male number: " << maleNum << endl;
        if (maleNum > 0) {
            cout << "Average male GPA: " << gpaSum / maleNum << endl;
            cout << "Average male height: " << heightSum / maleNum << endl;
        }
        cout << "Range query time: " << rangeElapsed.count() << " ms" << endl << endl;

        cout << "Delete test number: " << deleteNum << endl;
        cout << "Successful delete number: " << successDeleteNum << endl;
        cout << "Total deletion time: " << deleteElapsed.count() << " ms" << endl;
        cout << "Mean deletion time: " << deleteElapsed.count() / deleteNum << " ms" << endl << endl;

        cout << "Continue? (y/n) :" << endl;
        char c;
        cin >> c;
        if (c == 'n') {
            exit = true;
            cout << "Bye!" << endl;
        }
    }

    return 0;
}
