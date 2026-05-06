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

    Node(bool leaf) {
        isLeaf = leaf;
    }
};

class BTree {
private:
    Node* root;
    int d;
    int splitNum;

    int maxKeys() const {return d - 1;}

    int minKeys() const {return (d + 1) / 2 - 1;}

    int search(Node* node, int key) {
        int index = 0;

        while (index < node->keys.size() && key > node->keys[index]) {
            index++;
        }

        // key에 해당하는 값이 있으면 rid 반환
        if (index < node->keys.size() && key == node->keys[index])
            return node->rids[index];

        // leaf까지 갔는데 없으면 실패
        if (node->isLeaf)
            return -1;

        // 다음 node로 넘어가기
        return search(node->children[index], key);
    }

    void insert(Node* node, int key, int rid) {
        vector<Node*> path;
        vector<int> pathIndex;


        // 해당 key를 배치할 leaf까지 내려가기
        while (!node->isLeaf) {
            int index = 0;

            while (index < node->keys.size() && key > node->keys[index]) {
                index++;
            }

            // 만약 key에 해당하는 값이 이미 internal node에 있으면 rid 갱신
            if (index < node->keys.size() && key == node->keys[index]) {
                node->rids[index] = rid;
                return;
            }

            path.push_back(node);
            pathIndex.push_back(index);
            node = node->children[index];
        }


        // internal node에 key가 없었다면 leaf에 삽입
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

            int mid = node->keys.size() / 2;
            int midKey = node->keys[mid];
            int midRid = node->rids[mid];

            // 오른쪽에 새로운 node 만들기
            Node* right = new Node(node->isLeaf);

            // 새로운 node에 key와 rid 옮기기
            for (int i = mid + 1; i < node->keys.size(); i++) {
                right->keys.push_back(node->keys[i]);
                right->rids.push_back(node->rids[i]);
            }

            // internal node라면 새로운 node에 children도 옮기기
            if (!node->isLeaf) {
                for (int i = mid + 1; i < node->children.size(); i++) {
                    right->children.push_back(node->children[i]);
                }
                node->children.resize(mid + 1);
            }

            // 기존의 node에는 mid의 왼쪽 것들만 남기기
            node->keys.resize(mid);
            node->rids.resize(mid);

            // root를 split해야 하는 경우
            if (path.empty()) {
                Node* newRoot = new Node(false);
                newRoot->keys.push_back(midKey);
                newRoot->rids.push_back(midRid);
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
            parent->rids.insert(parent->rids.begin() + index, midRid);
            parent->children.insert(parent->children.begin() + index + 1, right);

            // path의 위쪽 parent로 넘어가기
            node = parent;
        }


    }

    // delete 할 때 underflow가 나는지 확인
    bool underflow(Node* node) {
        if (node == root)
            return false;

        return node->keys.size() < minKeys();
    }

    // delete 할 때 형제에게서 가져올 수 없다면 node 합치기
    void merge(Node* parent, int index) {
        Node* left = parent->children[index];
        Node* right = parent->children[index + 1];

        // 왼쪽에 해당 index의 부모 node 정보 넣기
        left->keys.push_back(parent->keys[index]);
        left->rids.push_back(parent->rids[index]);

        // 오른쪽 node 정보 전부 왼쪽에 추가
        for (int i = 0; i < right->keys.size(); i++) {
            left->keys.push_back(right->keys[i]);
            left->rids.push_back(right->rids[i]);
        }

        // leaf가 아닐 때는 children도 옮기기
        if (!right->isLeaf) {
            for (int i = 0; i < right->children.size(); i++) {
                left->children.push_back(right->children[i]);
            }
        }

        // 부모의 해당 index 삭제
        parent->keys.erase(parent->keys.begin() + index);
        parent->rids.erase(parent->rids.begin() + index);
        parent->children.erase(parent->children.begin() + index + 1);

        // 오른쪽 node 삭제
        delete right;
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
            if (leftSib->keys.size() > minKeys()) {
                // underflow난 node는 부모에게서 가져오고
                child->keys.insert(child->keys.begin(), parent->keys[index - 1]);
                child->rids.insert(child->rids.begin(), parent->rids[index - 1]);

                // 부모는 왼쪽 node에서 가져옴
                parent->keys[index - 1] = leftSib->keys.back();
                parent->rids[index - 1] = leftSib->rids.back();

                leftSib->keys.pop_back();
                leftSib->rids.pop_back();

                if (!leftSib->isLeaf) {
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
            if (rightSib->keys.size() > minKeys()) {
                child->keys.push_back(parent->keys[index]);
                child->rids.push_back(parent->rids[index]);

                parent->keys[index] = rightSib->keys.front();
                parent->rids[index] = rightSib->rids.front();

                rightSib->keys.erase(rightSib->keys.begin());
                rightSib->rids.erase(rightSib->rids.begin());

                if (!rightSib->isLeaf) {
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

        while (index < node->keys.size() && node->keys[index] < key) {
            index++;
        }

        if (index < node->keys.size() && node->keys[index] == key) {
            // leaf에 위치했다면 그냥 삭제
            if (node->isLeaf) {
                node->keys.erase(node->keys.begin() + index);
                node->rids.erase(node->rids.begin() + index);
                return;
            }

            // internal node에 위치해 있다면 왼쪽이나 오른쪽 subtree에서 가져와야 함.
            Node* left = node->children[index];
            Node* right = node->children[index + 1];

            // 왼쪽에서 가져오는 경우
            if (left->keys.size() > minKeys()) {
                Node* lNode = left;

                // 왼쪽 subtree에서 가장 큰 key 가져오기
                while (!lNode->isLeaf) {
                    lNode = lNode->children.back();
                }

                int lIndex = lNode->keys.size() - 1;
                int lKey = lNode->keys[lIndex];
                int lRid = lNode->rids[lIndex];

                node->keys[index] = lKey;
                node->rids[index] = lRid;

                remove(left, lKey);

                if (underflow(left))
                    fixUnderflow(node, index);
            }

            // 오른쪽에서 가져오는 경우
            else if (right->keys.size() > minKeys()) {
                Node* rNode = right;

                // 오른쪽 subtree에서 가장 작은 key 가져오기
                while (!rNode->isLeaf) {
                    rNode = rNode->children[0];
                }

                int rKey = rNode->keys[0];
                int rRid = rNode->rids[0];

                node->keys[index] = rKey;
                node->rids[index] = rRid;

                remove(right, rKey);

                if (underflow(right))
                    fixUnderflow(node, index + 1);
            }

            // 왼쪽, 오른쪽 모두 최소 key개수만큼 들고 있어서 가져올 수 없을 때는 merge
            else {
                merge(node, index);
                remove(left, key);
            }

            return;
        }

        // leaf까지 내려왔지만 delete하고 싶은 key가 없으면 그냥 return
        if (node->isLeaf)
            return;
        
        // 현재 node에 해당 key가 없으므로 다음 children으로 넘어감
        Node* child = node->children[index];
        remove(child, key);

        // underflow 확인 및 수정
        if (index < node->children.size() && underflow(node->children[index]))
            fixUnderflow(node, index);
    }

    void rangeQuery(Node* node, int startKey, int endKey, vector<int>& result) {
        if (node == nullptr)
            return;

        int i = 0;

        while (i < node->keys.size()) {
            if (!node->isLeaf)
                rangeQuery(node->children[i], startKey, endKey, result);

            if (node->keys[i] >= startKey && node->keys[i] <= endKey) 
                result.push_back(node->rids[i]);

            i++;
        }

        if (!node->isLeaf) 
            rangeQuery(node->children[i], startKey, endKey, result);
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
    BTree(int order) {
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
        rangeQuery(root, startKey, endKey, result);
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

        BTree b_tree(d);

        // insert 시간 측정
        auto insertStart = chrono::high_resolution_clock::now();
        for (int i = 0; i < students.size(); i++) {
            b_tree.insert(students[i].id, i);
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
            int rid = b_tree.search(key);

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
        vector<int> rangeRids = b_tree.rangeQuery(startKey, endKey);
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

            b_tree.remove(key);

            if (b_tree.search(key) == -1)
                successDeleteNum++;
        }
        auto deleteEnd = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> deleteElapsed = deleteEnd - deleteStart;

        // test 결과 출력
        cout << "Insertion time: " << insertElapsed.count() << " ms" << endl;
        cout << "Node utilization: " << b_tree.utilization() * 100 << "%" << endl;
        cout << "Number of splits: " << b_tree.getSplitNum() << endl << endl;

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
