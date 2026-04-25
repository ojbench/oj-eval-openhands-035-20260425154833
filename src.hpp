#pragma once
#include <stdexcept>
#include <cstring>

class MyString {
private:
    union {
        char* heap_ptr;
        char small_buffer[16];
    } storage{};
    size_t len_ = 0;
    size_t cap_ = 15; // excluding null when heap; for SSO, treated as 15
    bool sso_ = true;

    char* data_ptr() { return sso_ ? storage.small_buffer : storage.heap_ptr; }
    const char* data_ptr() const { return sso_ ? storage.small_buffer : storage.heap_ptr; }

    void switch_to_heap(size_t new_cap) {
        if (!sso_) return;
        size_t cap = new_cap;
        if (cap < 16) cap = 16; // ensure room when switching
        char* p = new char[cap + 1];
        if (len_) std::memcpy(p, storage.small_buffer, len_);
        p[len_] = '\0';
        storage.heap_ptr = p;
        cap_ = cap;
        sso_ = false;
    }

    void ensure_capacity(size_t need) {
        if (need <= capacity()) return;
        if (sso_ && need <= 15) return;
        size_t new_cap = capacity();
        if (new_cap < 16) new_cap = 16;
        while (new_cap < need) new_cap = new_cap + (new_cap >> 1); // 1.5x growth
        if (sso_) {
            switch_to_heap(new_cap);
        } else {
            char* np = new char[new_cap + 1];
            if (len_) std::memcpy(np, storage.heap_ptr, len_);
            np[len_] = '\0';
            delete[] storage.heap_ptr;
            storage.heap_ptr = np;
            cap_ = new_cap;
        }
    }

public:
    MyString() {
        sso_ = true;
        len_ = 0;
        cap_ = 15;
        storage.small_buffer[0] = '\0';
    }

    MyString(const char* s) {
        if (!s) {
            sso_ = true; len_ = 0; cap_ = 15; storage.small_buffer[0] = '\0';
            return;
        }
        size_t n = std::strlen(s);
        if (n <= 15) {
            sso_ = true; len_ = n; cap_ = 15;
            if (n) std::memcpy(storage.small_buffer, s, n);
            storage.small_buffer[n] = '\0';
        } else {
            sso_ = false; len_ = n; cap_ = n;
            storage.heap_ptr = new char[cap_ + 1];
            std::memcpy(storage.heap_ptr, s, n);
            storage.heap_ptr[n] = '\0';
        }
    }

    MyString(const MyString& other) {
        len_ = other.len_;
        sso_ = other.sso_;
        cap_ = other.sso_ ? 15 : other.cap_;
        if (sso_) {
            if (len_) std::memcpy(storage.small_buffer, other.storage.small_buffer, len_);
            storage.small_buffer[len_] = '\0';
        } else {
            storage.heap_ptr = new char[cap_ + 1];
            if (len_) std::memcpy(storage.heap_ptr, other.storage.heap_ptr, len_);
            storage.heap_ptr[len_] = '\0';
        }
    }

    MyString(MyString&& other) noexcept {
        len_ = other.len_;
        sso_ = other.sso_;
        cap_ = other.sso_ ? 15 : other.cap_;
        if (sso_) {
            if (len_) std::memcpy(storage.small_buffer, other.storage.small_buffer, len_);
            storage.small_buffer[len_] = '\0';
        } else {
            storage.heap_ptr = other.storage.heap_ptr;
            other.storage.heap_ptr = nullptr;
            other.cap_ = 15;
        }
        other.len_ = 0;
        other.sso_ = true;
        if (other.sso_) other.storage.small_buffer[0] = '\0';
    }

    MyString& operator=(MyString&& other) noexcept {
        if (this == &other) return *this;
        if (!sso_ && storage.heap_ptr) delete[] storage.heap_ptr;
        len_ = other.len_;
        sso_ = other.sso_;
        cap_ = other.sso_ ? 15 : other.cap_;
        if (sso_) {
            if (len_) std::memcpy(storage.small_buffer, other.storage.small_buffer, len_);
            storage.small_buffer[len_] = '\0';
        } else {
            storage.heap_ptr = other.storage.heap_ptr;
            other.storage.heap_ptr = nullptr;
        }
        other.len_ = 0;
        other.sso_ = true;
        other.cap_ = 15;
        other.storage.small_buffer[0] = '\0';
        return *this;
    }

    MyString& operator=(const MyString& other) {
        if (this == &other) return *this;
        if (!other.sso_) {
            ensure_capacity(other.len_);
            if (sso_) switch_to_heap(other.len_ < 16 ? 16 : other.len_);
            if (!sso_) {
                if (cap_ < other.len_) { ensure_capacity(other.len_); }
                if (other.len_) std::memcpy(storage.heap_ptr, other.data_ptr(), other.len_);
                storage.heap_ptr[other.len_] = '\0';
                len_ = other.len_;
            }
        } else {
            if (!sso_ && storage.heap_ptr) { delete[] storage.heap_ptr; }
            sso_ = true; cap_ = 15; len_ = other.len_;
            if (len_) std::memcpy(storage.small_buffer, other.storage.small_buffer, len_);
            storage.small_buffer[len_] = '\0';
        }
        return *this;
    }

    ~MyString() {
        if (!sso_ && storage.heap_ptr) {
            delete[] storage.heap_ptr;
            storage.heap_ptr = nullptr;
        }
    }

    const char* c_str() const { return data_ptr(); }
    size_t size() const { return len_; }
    size_t capacity() const { return sso_ ? (size_t)15 : cap_; }

    void reserve(size_t new_capacity) {
        if (new_capacity <= capacity()) return;
        if (new_capacity <= 15) return;
        if (sso_) {
            switch_to_heap(new_capacity);
        } else {
            char* np = new char[new_capacity + 1];
            if (len_) std::memcpy(np, storage.heap_ptr, len_);
            np[len_] = '\0';
            delete[] storage.heap_ptr;
            storage.heap_ptr = np;
            cap_ = new_capacity;
        }
    }

    void resize(size_t new_size) {
        if (new_size <= len_) {
            len_ = new_size;
            data_ptr()[len_] = '\0';
            return;
        }
        ensure_capacity(new_size);
        char* p = data_ptr();
        for (size_t i = len_; i < new_size; ++i) p[i] = '\0';
        len_ = new_size;
        p[len_] = '\0';
    }

    char& operator[](size_t index) {
        if (index >= len_) throw std::out_of_range("index out of range");
        return data_ptr()[index];
    }

    MyString operator+(const MyString& rhs) const {
        size_t total = len_ + rhs.len_;
        MyString res;
        if (total <= 15) {
            res.sso_ = true; res.len_ = total; res.cap_ = 15;
            if (len_) std::memcpy(res.storage.small_buffer, data_ptr(), len_);
            if (rhs.len_) std::memcpy(res.storage.small_buffer + len_, rhs.data_ptr(), rhs.len_);
            res.storage.small_buffer[total] = '\0';
        } else {
            res.sso_ = false; res.cap_ = total; res.len_ = total;
            res.storage.heap_ptr = new char[res.cap_ + 1];
            if (len_) std::memcpy(res.storage.heap_ptr, data_ptr(), len_);
            if (rhs.len_) std::memcpy(res.storage.heap_ptr + len_, rhs.data_ptr(), rhs.len_);
            res.storage.heap_ptr[total] = '\0';
        }
        return res;
    }

    void append(const char* str) {
        if (!str) return;
        size_t add = std::strlen(str);
        if (!add) return;
        ensure_capacity(len_ + add);
        char* p = data_ptr();
        std::memcpy(p + len_, str, add);
        len_ += add;
        p[len_] = '\0';
    }

    const char& at(size_t pos) const {
        if (pos >= len_) throw std::out_of_range("pos out of range");
        return data_ptr()[pos];
    }

    class const_iterator;
    class iterator {
        friend class MyString;
        char* ptr_ = nullptr;
        explicit iterator(char* p) : ptr_(p) {}
    public:
        iterator() = default;
        iterator& operator++() { ++ptr_; return *this; }
        iterator operator++(int) { iterator tmp(*this); ++(*this); return tmp; }
        iterator& operator--() { --ptr_; return *this; }
        iterator operator--(int) { iterator tmp(*this); --(*this); return tmp; }
        char& operator*() const { return *ptr_; }
        bool operator==(const iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const iterator& other) const { return ptr_ != other.ptr_; }
        bool operator==(const const_iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const const_iterator& other) const { return ptr_ != other.ptr_; }
    };

    class const_iterator {
        friend class MyString;
        const char* ptr_ = nullptr;
        explicit const_iterator(const char* p) : ptr_(p) {}
    public:
        const_iterator() = default;
        const_iterator& operator++() { ++ptr_; return *this; }
        const_iterator operator++(int) { const_iterator tmp(*this); ++(*this); return tmp; }
        const_iterator& operator--() { --ptr_; return *this; }
        const_iterator operator--(int) { const_iterator tmp(*this); --(*this); return tmp; }
        const char& operator*() const { return *ptr_; }
        bool operator==(const const_iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const const_iterator& other) const { return ptr_ != other.ptr_; }
        bool operator==(const iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const iterator& other) const { return ptr_ != other.ptr_; }
    };

    iterator begin() { return iterator(const_cast<char*>(data_ptr())); }
    iterator end() { return iterator(const_cast<char*>(data_ptr()) + len_); }
    const_iterator cbegin() const { return const_iterator(data_ptr()); }
    const_iterator cend() const { return const_iterator(data_ptr() + len_); }
};
