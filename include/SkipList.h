#ifndef TY_SKIPLIST
#define TY_SKIPLIST

#include <functional>
#include <string>
#include <fstream>
#include <iostream>
#include <random>
#include <mutex>

namespace ty
{
    template <typename K, typename V>
    class SkipList;

    template <typename K, typename V>
    class SkipListNode
    {
        friend class SkipList<K, V>;

    private:
        int level;
        SkipListNode<K, V> **forward;
        int *span;

    public:
        K key;
        V value;

        SkipListNode() {};
        SkipListNode(K k, V v, int level);
        ~SkipListNode();
    };

    template <typename K, typename V>
    SkipListNode<K, V>::SkipListNode(K k, V v, int lv) : key(k), value(v), level(lv)
    {
        this->forward = new SkipListNode<K, V> *[lv + 1];
        this->span = new int[lv + 1];
        std::fill(this->forward, this->forward + lv + 1, nullptr);
        // std::fill(this->span, this->span + lv + 1, 0);
        for (int i = 0; i < lv + 1; i++)
        {
            this->span[i] = 1;
        }
    }

    template <typename K, typename V>
    SkipListNode<K, V>::~SkipListNode()
    {
        if (this->forward != nullptr)
        {
            delete[] this->forward;
        }

        if (this->span != nullptr)
        {
            delete this->span;
        }
    }

    typedef enum
    {
        SKIPLSIT_SELECT_SUCCESS,
        SKIPLIST_INSRT_SUCCESS,
        SKIPLIST_INSRT_FAIL,
        SKIPLIST_ERASE_SUCCESS,
        SKIPLIST_KEY_EXIST,
        SKIPLIST_KEY_NOT_EXIST,
        SKIPLIST_OPEN_FILE_FAIL,
        SKIPLIST_LOAD_SUCCESS,
        SKIPLIST_SAVE_SUCCESS,
    } SKIPLIST_STATE;

    template <typename K, typename V>
    class SkipList
    {
    private:
        static constexpr int P = 2;
        static constexpr int MAX_LEVEL = 32;
        int length;
        int current_level;
        SkipListNode<K, V> *header;
        std::ifstream file_reader;
        std::ofstream file_wirter;

        std::default_random_engine rand_engine;

        std::mutex mtx;

        void clear(SkipListNode<K, V> *SkipListNode);
        void init();
        int get_random_level();

    public:
        SkipList();
        ~SkipList();
        SKIPLIST_STATE insert(K k, V v);
        SKIPLIST_STATE select(K k, V *v);
        SKIPLIST_STATE erase(K k);
        V at(size_t index);
        size_t size();
        SKIPLIST_STATE save(const char *file);
        SKIPLIST_STATE save(const char *file, std::function<void(std::ofstream &fp, K, V)> func);
        SKIPLIST_STATE load(const char *file);
        SKIPLIST_STATE load(const char *file, std::function<bool(std::ifstream &fp, K&, V&)> func);
    };

    template <typename K, typename V>
    SkipList<K, V>::SkipList()
    {
        init();
    }

    template <typename K, typename V>
    SkipList<K, V>::~SkipList()
    {
        if (this->file_reader.is_open())
        {
            this->file_reader.close();
        }

        if (this->file_wirter.is_open())
        {
            this->file_wirter.close();
        }

        if (this->header->forward[0] != nullptr)
        {
            clear(this->header->forward[0]);
        }

        delete this->header;
    }

    template <typename K, typename V>
    void SkipList<K, V>::clear(SkipListNode<K, V> *SkipListNode)
    {
        while (SkipListNode != nullptr)
        {
            auto temp = SkipListNode->forward[0];
            delete SkipListNode;
            SkipListNode = temp;
        }
    }

    template <typename K, typename V>
    void ty::SkipList<K, V>::init()
    {
        this->current_level = 0;
        this->length = 0;
        K k{};
        V v{};
        this->header = new SkipListNode<K, V>(k, v, this->MAX_LEVEL);
    }

    template <typename K, typename V>
    int SkipList<K, V>::get_random_level()
    {
        int level = 0;
        std::uniform_int_distribution<int> u(1, this->P);
        while (u(this->rand_engine) == 1)
        {
            ++level;
        }
        return level < this->MAX_LEVEL ? level : this->MAX_LEVEL;
    }

    template <typename K, typename V>
    SKIPLIST_STATE SkipList<K, V>::insert(K k, V v)
    {
        this->mtx.lock();
        SkipListNode<K, V> *current = this->header;
        SkipListNode<K, V> *update[this->MAX_LEVEL + 1] = {nullptr};
        int node_position[this->MAX_LEVEL + 1] = {0};
        int pos = 0;
        for (int i = this->current_level; i >= 0; --i)
        {
            while (current->forward[i] != nullptr && current->forward[i]->key < k)
            {
                pos += current->span[i];
                current = current->forward[i];
            }

            update[i] = current;
            node_position[i] = pos;
        }

        pos += current->span[0];
        current = current->forward[0];
        // 找到key已存在 则 修改内容
        if (current != nullptr && current->key == k)
        {
            current->value = v;
        }

        if (current == nullptr || current->key != k)
        {
            int level = this->get_random_level();

            if (level > this->current_level)
            {
                for (int i = this->current_level + 1; i <= level; ++i)
                {
                    update[i] = this->header;
                    node_position[i] = 0;
                    update[i]->span[i] = pos;
                }
                this->current_level = level;
            }

            SkipListNode<K, V> *node = new SkipListNode<K, V>(k, v, level);

            for (int i = 0; i <= level; i++)
            {
                node->forward[i] = update[i]->forward[i];
                node->span[i] = node_position[i] + update[i]->span[i] + 1 - pos;
                update[i]->forward[i] = node;
                update[i]->span[i] = pos - node_position[i];
            }

            for (int i = level + 1; i <= this->current_level; i++)
            {
                update[i]->span[i] += 1;
            }

            ++this->length;
        }

        this->mtx.unlock();
        return SKIPLIST_INSRT_SUCCESS;
    }

    template <typename K, typename V>
    SKIPLIST_STATE SkipList<K, V>::select(K k, V *v)
    {
        SkipListNode<K, V> *current = this->header;
        for (int i = this->current_level; i >= 0; --i)
        {
            while (current->forward[i] != nullptr && current->forward[i]->key < k)
            {
                current = current->forward[i];
            }
        }

        current = current->forward[0];

        if (current == nullptr || current->key != k)
        {
            return SKIPLIST_KEY_NOT_EXIST;
        }

        *v = current->value;

        return SKIPLSIT_SELECT_SUCCESS;
    }

    template <typename K, typename V>
    SKIPLIST_STATE SkipList<K, V>::erase(K k)
    {
        this->mtx.lock();
        SkipListNode<K, V> *current = this->header;
        SkipListNode<K, V> *update[this->MAX_LEVEL + 1] = {nullptr};
        int node_position[this->MAX_LEVEL + 1] = {0};
        int pos = 0;
        for (int i = this->current_level; i >= 0; --i)
        {
            while (current->forward[i] != nullptr && current->forward[i]->key < k)
            {
                pos += current->span[i];
                current = current->forward[i];
            }
            update[i] = current;
            node_position[i] = pos;
        }

        pos += current->span[0];
        current = current->forward[0];
        if (current == nullptr || current->key != k)
        {
            this->mtx.unlock();
            return SKIPLIST_KEY_NOT_EXIST;
        }

        for (int i = 0; i <= this->current_level; ++i)
        {
            if (update[i]->forward[i] != current)
            {
                update[i]->span[i] -= 1;
            }
            else
            {
                update[i]->forward[i] = current->forward[i];
                update[i]->span[i] = update[i]->span[i] + current->span[i] - 1;
            }
        }

        delete current;

        while (this->current_level > 0 && this->header->forward[this->current_level] == nullptr)
        {
            this->header->span[this->current_level] = 1;
            --this->current_level;
        }

        --this->length;
        this->mtx.unlock();
        return SKIPLIST_ERASE_SUCCESS;
    }

    template <typename K, typename V>
    V SkipList<K, V>::at(size_t index)
    {
        if (index >= this->length)
        {
            throw std::out_of_range("index out of range!");
        }
        SkipListNode<K, V> *current = this->header;
        int pos = 0;
        for (int i = this->current_level; i >= 0 && pos < index + 1; --i)
        {
            while (current->forward[i] != nullptr && current->span[i] + pos < index + 1)
            {
                pos += current->span[i];
                current = current->forward[i];
            }
        }

        return current->forward[0]->value;
    }

    template <typename K, typename V>
    inline size_t SkipList<K, V>::size()
    {
        return this->length;
    }

    template <typename K, typename V>
    SKIPLIST_STATE SkipList<K, V>::save(const char *file)
    {
        if (this->file_wirter.is_open())
        {
            this->file_wirter.close();
        }
        this->file_wirter.open(file, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
        if (!this->file_wirter.is_open())
        {
            return SKIPLIST_OPEN_FILE_FAIL;
        }

        for (SkipListNode<K, V> *i = this->header->forward[0]; i != nullptr; i = i->forward[0])
        {
            this->file_wirter.write(reinterpret_cast<char *>(&(i->key)), sizeof(i->key));
            this->file_wirter.write(reinterpret_cast<char *>(&(i->value)), sizeof(i->value));
        }

        return SKIPLIST_SAVE_SUCCESS;
    }

    template <typename K, typename V>
    SKIPLIST_STATE SkipList<K, V>::save(const char *file, std::function<void(std::ofstream &fp, K, V)> func)
    {
        if (this->file_wirter.is_open())
        {
            this->file_wirter.close();
        }
        this->file_wirter.open(file, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
        if (!this->file_wirter.is_open())
        {
            return SKIPLIST_OPEN_FILE_FAIL;
        }

        for (SkipListNode<K, V> *i = this->header->forward[0]; i != nullptr; i = i->forward[0])
        {
            func(this->file_wirter, i->key, i->value);
        }

        return SKIPLIST_SAVE_SUCCESS;
    }

    template <typename K, typename V>
    SKIPLIST_STATE SkipList<K, V>::load(const char *file)
    {
        if (this->file_reader.is_open())
        {
            this->file_reader.close();
        }
        this->file_reader.open(file, std::ios_base::in | std::ios_base::binary);
        if (!this->file_reader.is_open())
        {
            return SKIPLIST_OPEN_FILE_FAIL;
        }

        clear(this->header);
        init();

        K k{};
        V v{};

        while (!this->file_reader.eof())
        {
            this->file_reader.read(reinterpret_cast<char *>(&k), sizeof(k));
            this->file_reader.read(reinterpret_cast<char *>(&v), sizeof(v));
            this->insert(k, v);
        }

        return SKIPLIST_LOAD_SUCCESS;
    }

    template <typename K, typename V>
    SKIPLIST_STATE SkipList<K, V>::load(const char *file, std::function<bool(std::ifstream &fp, K&, V&)> func)
    {
        if (this->file_reader.is_open())
        {
            this->file_reader.close();
        }
        this->file_reader.open(file, std::ios_base::in | std::ios_base::binary);
        if (!this->file_reader.is_open())
        {
            return SKIPLIST_OPEN_FILE_FAIL;
        }

        clear(this->header);
        init();

        K k{};
        V v{};

        while (!this->file_reader.eof() && func(this->file_reader, k, v))
        {
            
            this->insert(k, v);
        }

        return SKIPLIST_LOAD_SUCCESS;
    }

}

#endif