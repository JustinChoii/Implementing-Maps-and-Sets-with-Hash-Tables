// Submitter: justinic(Choi, Justin)
// Partner  : brgallar(Gallardo, Brayan)
// We certify that we worked cooperatively on this programming
//   assignment, according to the rules for pair programming
#ifndef HASH_SET_HPP_
#define HASH_SET_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"
#include "pair.hpp"


namespace ics {


#ifndef undefinedhashdefined
#define undefinedhashdefined
template<class T>
int undefinedhash (const T& a) {return 0;}
#endif /* undefinedhashdefined */

//Instantiate the templated class supplying thash(a): produces a hash value for a.
//If thash is defaulted to undefinedhash in the template, then a constructor must supply chash.
//If both thash and chash are supplied, then they must be the same (by ==) function.
//If neither is supplied, or both are supplied but different, TemplateFunctionError is raised.
//The (unique) non-undefinedhash value supplied by thash/chash is stored in the instance variable hash.
template<class T, int (*thash)(const T& a) = undefinedhash<T>> class HashSet {
  public:
    typedef int (*hashfunc) (const T& a);

    //Destructor/Constructors
    ~HashSet ();

    HashSet (double the_load_threshold = 1.0, int (*chash)(const T& a) = nullptr);
    explicit HashSet (int initial_bins, double the_load_threshold = 1.0, int (*chash)(const T& k) = nullptr);
    HashSet (const HashSet<T,thash>& to_copy, double the_load_threshold = 1.0, int (*chash)(const T& a) = nullptr);
    explicit HashSet (const std::initializer_list<T>& il, double the_load_threshold = 1.0, int (*chash)(const T& a) = nullptr);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit HashSet (const Iterable& i, double the_load_threshold = 1.0, int (*chash)(const T& a) = nullptr);


    //Queries
    bool empty      () const;
    int  size       () const;
    bool contains   (const T& element) const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    bool contains_all (const Iterable& i) const;


    //Commands
    int  insert (const T& element);
    int  erase  (const T& element);
    void clear  ();

    //Iterable class must support "for" loop: .begin()/.end() and prefix ++ on returned result

    template <class Iterable>
    int insert_all(const Iterable& i);

    template <class Iterable>
    int erase_all(const Iterable& i);

    template<class Iterable>
    int retain_all(const Iterable& i);


    //Operators
    HashSet<T,thash>& operator = (const HashSet<T,thash>& rhs);
    bool operator == (const HashSet<T,thash>& rhs) const;
    bool operator != (const HashSet<T,thash>& rhs) const;
    bool operator <= (const HashSet<T,thash>& rhs) const;
    bool operator <  (const HashSet<T,thash>& rhs) const;
    bool operator >= (const HashSet<T,thash>& rhs) const;
    bool operator >  (const HashSet<T,thash>& rhs) const;

    template<class T2, int (*hash2)(const T2& a)>
    friend std::ostream& operator << (std::ostream& outs, const HashSet<T2,hash2>& s);



  private:
    class LN;

  public:
    class Iterator {
      public:
        typedef pair<int,LN*> Cursor;

        //Private constructor called in begin/end, which are friends of HashSet<T,thash>
        ~Iterator();
        T           erase();
        std::string str  () const;
        HashSet<T,thash>::Iterator& operator ++ ();
        HashSet<T,thash>::Iterator  operator ++ (int);
        bool operator == (const HashSet<T,thash>::Iterator& rhs) const;
        bool operator != (const HashSet<T,thash>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const HashSet<T,thash>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator HashSet<T,thash>::begin () const;
        friend Iterator HashSet<T,thash>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        Cursor              current; //Bin Index and Cursor; stops if LN* == nullptr
        HashSet<T,thash>*   ref_set;
        int                 expected_mod_count;
        bool                can_erase = true;

        //Helper methods
        void advance_cursors();

        //Called in friends begin/end
        Iterator(HashSet<T,thash>* iterate_over, bool from_begin);
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    class LN {
      public:
        LN ()                      {}
        LN (const LN& ln)          : value(ln.value), next(ln.next){}
        LN (T v,  LN* n = nullptr) : value(v), next(n){}

        T   value;
        LN* next   = nullptr;
    };

public:
  int (*hash)(const T& k);   //Hashing function used (from template or constructor)
private:
  LN** set      = nullptr;   //Pointer to array of pointers: each bin stores a list with a trailer node
  double load_threshold;     //used/bins <= load_threshold
  int bins      = 1;         //# bins in array (should start >= 1 so hash_compress doesn't % 0)
  int used      = 0;         //Cache for number of key->value pairs in the hash table
  int mod_count = 0;         //For sensing concurrent modification


  //Helper methods
  int   hash_compress        (const T& key)              const;  //hash function ranged to [0,bins-1]
  LN*   find_element         (const T& element)          const;  //Returns reference to element's node or nullptr
  LN*   copy_list            (LN*   l)                   const;  //Copy the elements in a bin (order irrelevant)
  LN**  copy_hash_table      (LN** ht, int bins)         const;  //Copy the bins/keys/values in ht tree (order in bins irrelevant)

  void  ensure_load_threshold(int new_used);                     //Reallocate if load_threshold > load_threshold
  void  delete_hash_table    (LN**& ht, int bins);               //Deallocate all LN in ht (and the ht itself; ht == nullptr)
};





//HashSet class and related definitions

////////////////////////////////////////////////////////////////////////////////
//
//Destructor/Constructors

template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::~HashSet() {
    delete_hash_table(set,bins);
}

template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(double the_load_threshold, int (*chash)(const T& element))
: hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold){
    if (hash == nullptr)
        throw TemplateFunctionError("default constructor: neither specified");
    if (thash != nullptr && chash != nullptr && chash != thash)
        throw TemplateFunctionError("both given but different");
    set = new LN*[bins];
    for (int i =0; i <bins; ++i)
        set[i] = new LN();
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(int initial_bins, double the_load_threshold, int (*chash)(const T& element))
: hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold), bins(initial_bins) {
    if (hash == nullptr) {
        throw TemplateFunctionError("not specified");
    }
    if (thash != nullptr && chash != nullptr && chash != thash) {
        throw TemplateFunctionError("both given but different");
    }
    if (bins == 0) {
        bins = 1;
    }
    set = new LN*[bins];
    for (int i =0; i <bins; ++i)
        set[i] = new LN();
}



template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(const HashSet<T,thash>& to_copy, double the_load_threshold, int (*chash)(const T& element))
: hash(thash != nullptr ? thash : chash), bins(to_copy.bins), load_threshold(the_load_threshold) {
    if (hash == nullptr) {
        hash = to_copy.hash;
    }
    if (thash != nullptr && chash != nullptr && thash != chash) {
        throw TemplateFunctionError("both specified and different");
    }
    if (hash == to_copy.hash && to_copy.size() == size()) {
        used = to_copy.used;
        set  = copy_hash_table(to_copy.set, to_copy.bins);
    }else {
        bins = int(to_copy.size());
        set = new LN*[bins];
        for (int b=0; b<bins; ++b)
            set[b] = new LN();
        for (int i=0; i<to_copy.bins; i++) {
            LN *temp = to_copy.set[i];
            while (temp->next != nullptr) {
                insert(temp->value);
                temp = temp->next;
            }
        }
    }
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(const std::initializer_list<T>& il, double the_load_threshold, int (*chash)(const T& element))
: hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold), bins(std::max(1,int(il.size()/the_load_threshold))) {
    if (hash == nullptr) {
        throw TemplateFunctionError("neither specified");
    }
    if (thash != nullptr && chash != nullptr && thash != chash) {
        throw TemplateFunctionError("both specified and different");
    }
    set = new LN* [bins];
    for (int b=0; b<bins; ++b) {
        set[b] = new LN();
    }
    for (const T& s_elem : il) {
        insert(s_elem);
    }
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
HashSet<T,thash>::HashSet(const Iterable& i, double the_load_threshold, int (*chash)(const T& a))
: hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold), bins(std::max(1,int(i.size()/the_load_threshold))) {
    if (hash == nullptr)
        throw TemplateFunctionError("HashSet::Iterable constructor: neither specified");
    if (thash != nullptr && chash != nullptr && thash != chash)
        throw TemplateFunctionError("HashSet::Iterable constructor: both specified and different");

    set = new LN* [bins];
    for (int b=0; b<bins; ++b)
        set[b] = new LN();

    for (const T& v : i)
        insert(v);
}

////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::empty() const {
    return used == 0;
}


template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::size() const {
    return used;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::contains (const T& element) const {
    return find_element(element) != nullptr;
}


template<class T, int (*thash)(const T& a)>
std::string HashSet<T,thash>::str() const {
    std::ostringstream answer;
    answer << "HashSet[]";
    return answer.str();
}



template<class T, int (*thash)(const T& a)>
template <class Iterable>
bool HashSet<T,thash>::contains_all(const Iterable& i) const {
    for (const T& v : i)
        if (!contains(v))
            return false;
    return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::insert(const T& element) {
    if (find_element(element) == nullptr)
    {
        ensure_load_threshold(used+1);
        ++used;
        ++mod_count;
        int bin = hash_compress(element);
        set[bin] = new LN(element, set[bin]);
        return 1;
    } else {
        return 0;
    }

}



template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::erase(const T& element) {
    LN* temp = find_element(element);
    if (temp != nullptr) {
        LN* to_delete = temp->next;
        *temp = *temp->next;
        delete to_delete;
        used--;
        mod_count++;
        return 1;
    } else {
        return 0;
    }
}


template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::clear() {
    int i = 0;
    while (i < bins) {
        LN* node = set[i];
        while (node->next != nullptr) {
            LN* to_delete = node;
            node = node->next;
            delete to_delete;
        }
        set[i] = node;
        ++i;
    }
    used = 0;
    ++mod_count;
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
int HashSet<T,thash>::insert_all(const Iterable& i) {
    int count = 0;
    for (const T& v : i)
        count += insert(v);

    return count;
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
int HashSet<T,thash>::erase_all(const Iterable& i) {
    int count = 0;
    for (const T& v : i)
        count += erase(v);

    return count;
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
int HashSet<T,thash>::retain_all(const Iterable& i) {

    return used;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T, int (*thash)(const T& a)>
HashSet<T,thash>& HashSet<T,thash>::operator = (const HashSet<T,thash>& rhs) {
    if (this == &rhs)
        return *this;

    return *this;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator == (const HashSet<T,thash>& rhs) const {
    if (this == &rhs)
        return true;
    if (used != rhs.size())
        return false;
    for (int i = 0; i < bins; i++) {
        LN *temp = set[i];
        while (temp -> next != nullptr) {
            if (!rhs.contains(temp -> value)) {
                return false;
            }
            temp = temp -> next;
        }

    }
    return true;
    }


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator != (const HashSet<T,thash>& rhs) const {
    return !(*this == rhs);
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator <= (const HashSet<T,thash>& rhs) const {
    if (this == &rhs)
        return true;
    if (used > rhs.size())
         return false;

    for (int i = 0; i <bins; ++i) {
        LN* temp = set[i];
        while( temp -> next != nullptr) {
            if (!rhs.contains(temp ->value)) {
                return false;
            }
            temp = temp -> next;
        }
    }
    return true;
}

template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator < (const HashSet<T,thash>& rhs) const {
    if (this == &rhs)
        return false;
    if (used >= rhs.size())
        return false;

    for (int i = 0; i <bins; ++i) {
        LN* temp = set[i];
        while( temp -> next != nullptr) {
            if (!rhs.contains(temp ->value)) {
                return false;
            }
            temp = temp -> next;
        }
    }
    return true;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator >= (const HashSet<T,thash>& rhs) const {
    return rhs <= *this;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator > (const HashSet<T,thash>& rhs) const {
    return rhs < *this;
}



template<class T, int (*thash)(const T& a)>
std::ostream& operator << (std::ostream& outs, const HashSet<T,thash>& s) {
    outs << "set[]";
    return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class T, int (*thash)(const T& a)>
auto HashSet<T,thash>::begin () const -> HashSet<T,thash>::Iterator {
    return Iterator(const_cast<HashSet<T,thash>*>(this),true);
}


template<class T, int (*thash)(const T& a)>
auto HashSet<T,thash>::end () const -> HashSet<T,thash>::Iterator {
    return Iterator(const_cast<HashSet<T,thash>*>(this),false);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::hash_compress (const T& element) const {
    return abs(hash(element))%bins;
}


template<class T, int (*thash)(const T& a)>
typename HashSet<T,thash>::LN* HashSet<T,thash>::find_element (const T& element) const {
    int bin = hash_compress(element);
    for (LN* c = set[bin]; c->next != nullptr; c = c->next) {
        if (element == c->value) {
            return c;
        }
    }
    return nullptr;
}

template<class T, int (*thash)(const T& a)>
typename HashSet<T,thash>::LN* HashSet<T,thash>::copy_list (LN* l) const {
    if (l == nullptr){
        return nullptr;
    }
    if (l -> next == nullptr){
        return new LN();
    }
    LN* to_return = new LN(l -> value, new LN());
    LN* temp = l -> next;
    while ( temp -> next != nullptr){
        to_return = new LN(temp -> value, to_return);
        temp = temp -> next;
    }
    return to_return;

}


template<class T, int (*thash)(const T& a)>
typename HashSet<T,thash>::LN** HashSet<T,thash>::copy_hash_table (LN** ht, int bins) const {
    LN** new_ht = new LN*[bins];
    for (int i=0; i<bins; i++) {
        new_ht[i] = copy_list(ht[i]);
    }
    return new_ht;
}


template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::ensure_load_threshold(int new_used) {
    if (new_used > load_threshold * bins) {
        LN **oldset = set;
        int oldbins = bins;
        bins = 2 * oldbins;
        set = new LN *[bins];
        for (int i = 0; i < bins; ++i)
            set[i] = new LN();
        for (int i = 0; i < oldbins; ++i) {
            LN *c = oldset[i];
            for (; c->next != nullptr;) {
                int bin = hash_compress(c->value);
                LN *to_move = c;
                c = c->next;
                to_move->next = set[bin];
                set[bin] = to_move;
            }
            delete c;
        }

        delete[] oldset;
    }

    return;
}


template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::delete_hash_table (LN**& ht, int bins) {
    for (int i=0; i<bins; ++i) {
        LN *temp = ht[i];
        while (temp->next != nullptr) {
            LN *to_delete = temp;
            temp = temp->next;
            delete to_delete;
        }
    }
    delete[] ht;
    ht = nullptr;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::Iterator::advance_cursors() {
    if (current.second != nullptr && current.second->next != nullptr && current.second->next->next != nullptr) {
        current.second = current.second->next;
        return;
    }
    for (int i = current.first + 1; i < ref_set->bins; ++i) {
        if (ref_set->set[i]->next != nullptr) {
            current.second = ref_set->set[i];
            current.first = i;
            return;
        }
    }
    current.second = nullptr;
    current.first  = -1;
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::Iterator::Iterator(HashSet<T,thash>* iterate_over, bool begin)
: ref_set(iterate_over) {
    current = ics::pair<int,LN*>(-1, nullptr);
    expected_mod_count = ref_set->mod_count;
    if (begin) {
        advance_cursors();
    }
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::Iterator::~Iterator()
{}


template<class T, int (*thash)(const T& a)>
T HashSet<T,thash>::Iterator::erase() {
    if (expected_mod_count != ref_set->mod_count) {
        throw ConcurrentModificationError("HashSet::Iterator::erase");
    }
    if (!can_erase) {
        throw CannotEraseError("HashSet::Iterator::erase Iterator cursor already erased");
    }
    if (current.second == nullptr) {
        throw CannotEraseError("HashSet::Iterator::erase Iterator cursor beyond data structure");
    }
    can_erase = false;
    T returnentry = current.second->value;
    ref_set -> used --;
    ref_set -> mod_count++;
    LN* to_delete = current.second->next;
    *current.second = *(current.second->next);
    expected_mod_count = ref_set->mod_count;
    delete to_delete;
    return returnentry;
}

template<class T, int (*thash)(const T& a)>
std::string HashSet<T,thash>::Iterator::str() const {
    std::ostringstream string;
    string << ref_set->str() << "(current=" << current.first << "/" << current.second << ",expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
    return string.str();
}

template<class T, int (*thash)(const T& a)>
auto  HashSet<T,thash>::Iterator::operator ++ () -> HashSet<T,thash>::Iterator& {
    if (expected_mod_count != ref_set -> mod_count) {
        throw ConcurrentModificationError("HashSet::Iterator::operator ++");
    }
    if (current.second == nullptr) {
        return *this;
    }
    if (current.second->next == nullptr) {
        advance_cursors();
    }
    if (can_erase) {
        advance_cursors();
    }
    can_erase = true;
    return *this;
}


template<class T, int (*thash)(const T& a)>
auto  HashSet<T,thash>::Iterator::operator ++ (int) -> HashSet<T,thash>::Iterator {
    Iterator* to_return = new Iterator(*this);
    if (expected_mod_count != ref_set->mod_count) {
        throw ConcurrentModificationError("HashSet::Iterator::operator ++(int)");
    }
    if (current.second == nullptr) {
        return *to_return;
    }
    if (current.second->next == nullptr) {
        advance_cursors();
    }
    if (can_erase) {
        advance_cursors();
    }
    can_erase = true;
    return *to_return;
}


    template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::Iterator::operator == (const HashSet<T,thash>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if (rhsASI == 0) {
        throw IteratorTypeError("HashSet::Iterator::operator ==");
    }
    if (ref_set != rhsASI->ref_set) {
        throw ComparingDifferentIteratorsError("HashSet::Iterator::operator ==");
    } else {
        return this->current.second == rhsASI->current.second;
    }
}

template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::Iterator::operator != (const HashSet<T,thash>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if (rhsASI == 0) {
        throw IteratorTypeError("HashSet::Iterator::operator !=");
    }
    if (ref_set != rhsASI->ref_set) {
        throw ComparingDifferentIteratorsError("HashSet::Iterator::operator !=");
    } else {
        return this->current.second != rhsASI->current.second;
    }
}

template<class T, int (*thash)(const T& a)>
T& HashSet<T,thash>::Iterator::operator *() const {
    if (!can_erase) {
        throw IteratorPositionIllegal("Iterator illegal");
    }
    if (current.second == nullptr) {
        throw IteratorPositionIllegal("Iterator illegal");
    }
    if (expected_mod_count != ref_set->mod_count) {
        throw ConcurrentModificationError("HashSet::iterator::operator *");
    } else {
        return current.second->value;
    }
}

template<class T, int (*thash)(const T& a)>
T* HashSet<T,thash>::Iterator::operator ->() const {
    if (current.second == nullptr) {
        throw IteratorPositionIllegal("Iterator illegal");
    }
    if (!can_erase) {
        throw IteratorPositionIllegal("Iterator illegal");
    }
    if (expected_mod_count != ref_set->mod_count) {
        throw ConcurrentModificationError("operator ->");
    } else {
        return &(current.second -> value);
    }
}

}

#endif /* HASH_SET_HPP_ */
