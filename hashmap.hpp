// Submitter: justinic(Choi, Justin)
// Partner  : brgallar(Gallardo, Brayan)
// We certify that we worked cooperatively on this programming
//   assignment, according to the rules for pair programming
#ifndef HASH_MAP_HPP_
#define HASH_MAP_HPP_

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
template<class KEY,class T, int (*thash)(const KEY& a) = nullptr> class HashMap {
  public:
    typedef ics::pair<KEY,T>   Entry;
    typedef int (*hashfunc) (const KEY& a);

    //Destructor/Constructors
    ~HashMap ();

    HashMap          (double the_load_threshold = 1.0, int (*chash)(const KEY& a) = nullptr);
    explicit HashMap (int initial_bins, double the_load_threshold = 1.0, int (*chash)(const KEY& k) = nullptr);
    HashMap          (const HashMap<KEY,T,thash>& to_copy, double the_load_threshold = 1.0, int (*chash)(const KEY& a) = nullptr);
    explicit HashMap (const std::initializer_list<Entry>& il, double the_load_threshold = 1.0, int (*chash)(const KEY& a) = nullptr);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit HashMap (const Iterable& i, double the_load_threshold = 1.0, int (*chash)(const KEY& a) = nullptr);


    //Queries
    bool empty      () const;
    int  size       () const;
    bool has_key    (const KEY& key) const;
    bool has_value  (const T& value) const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<


    //Commands
    T    put   (const KEY& key, const T& value);
    T    erase (const KEY& key);
    void clear ();

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    int put_all(const Iterable& i);


    //Operators

    T&       operator [] (const KEY&);
    const T& operator [] (const KEY&) const;
    HashMap<KEY,T,thash>& operator = (const HashMap<KEY,T,thash>& rhs);
    bool operator == (const HashMap<KEY,T,thash>& rhs) const;
    bool operator != (const HashMap<KEY,T,thash>& rhs) const;

    template<class KEY2,class T2, int (*hash2)(const KEY2& a)>
    friend std::ostream& operator << (std::ostream& outs, const HashMap<KEY2,T2,hash2>& m);



  private:
    class LN;

  public:
    class Iterator {
      public:
         typedef pair<int,LN*> Cursor;

        //Private constructor called in begin/end, which are friends of HashMap<T>
        ~Iterator();
        Entry       erase();
        std::string str  () const;
        HashMap<KEY,T,thash>::Iterator& operator ++ ();
        HashMap<KEY,T,thash>::Iterator  operator ++ (int);
        bool operator == (const HashMap<KEY,T,thash>::Iterator& rhs) const;
        bool operator != (const HashMap<KEY,T,thash>::Iterator& rhs) const;
        Entry& operator *  () const;
        Entry* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const HashMap<KEY,T,thash>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator HashMap<KEY,T,thash>::begin () const;
        friend Iterator HashMap<KEY,T,thash>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        Cursor                current; //Bin Index and Cursor; stops if LN* == nullptr
        HashMap<KEY,T,thash>* ref_map;
        int                   expected_mod_count;
        bool                  can_erase = true;

        //Helper methods
        void advance_cursors();

        //Called in friends begin/end
        Iterator(HashMap<KEY,T,thash>* iterate_over, bool from_begin);
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    class LN {
    public:
      LN ()                         : next(nullptr){}
      LN (const LN& ln)             : value(ln.value), next(ln.next){}
      LN (Entry v, LN* n = nullptr) : value(v), next(n){}

      Entry value;
      LN*   next;
  };

  int (*hash)(const KEY& k);  //Hashing function used (from template or constructor)
  LN** map      = nullptr;    //Pointer to array of pointers: each bin stores a list with a trailer node
  double load_threshold;      //used/bins <= load_threshold
  int bins      = 1;          //# bins in array (should start >= 1 so hash_compress doesn't % 0)
  int used      = 0;          //Cache for number of key->value pairs in the hash table
  int mod_count = 0;          //For sensing concurrent modification


  //Helper methods
  int   hash_compress        (const KEY& key)          const;  //hash function ranged to [0,bins-1]
  LN*   find_key             (const KEY& key) const;           //Returns reference to key's node or nullptr
  LN*   copy_list            (LN*   l)                 const;  //Copy the keys/values in a bin (order irrelevant)
  LN**  copy_hash_table      (LN** ht, int bins)       const;  //Copy the bins/keys/values in ht tree (order in bins irrelevant)

  void  ensure_load_threshold(int new_used);                   //Reallocate if load_factor > load_threshold
  void  delete_hash_table    (LN**& ht, int bins);             //Deallocate all LN in ht (and the ht itself; ht == nullptr)
};





////////////////////////////////////////////////////////////////////////////////
//
//HashMap class and related definitions

//Destructor/Constructors
template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::~HashMap() {
    delete_hash_table(map, bins);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(double the_load_threshold, int (*chash)(const KEY& k))
:   hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold){
    if (hash == nullptr){
        throw TemplateFunctionError("HashMap::default constructor: neither specified");
    }
    if(thash != nullptr and chash != nullptr and thash != chash){
        throw TemplateFunctionError("HashMap::default constructor: both specified and different");
    }
    map = new LN* [bins];
    for (int i = 0; i < bins; i++){
        map[i] = new LN();
    }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(int initial_bins, double the_load_threshold, int (*chash)(const KEY& k))
:   hash(thash != nullptr ? thash : chash), bins (initial_bins), load_threshold(the_load_threshold){
    if (hash == nullptr){
        throw TemplateFunctionError("HashMap::length constructor: neither specified");
    }
    if (thash != nullptr and chash != nullptr and thash != chash){
        throw TemplateFunctionError("HashMap::length constructor: both specified and different");
    }
    if (bins <= 0){
        bins = 1;
    }
    map = new LN* [bins];
    for (int i = 0; i < bins; i ++){
        map[i] = new LN();
    }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(const HashMap<KEY,T,thash>& to_copy, double the_load_threshold, int (*chash)(const KEY& a))
:   hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold), bins(to_copy.bins)
{
    if (hash == nullptr){
        hash = to_copy.hash;
    }
    if (thash != nullptr and chash != nullptr and thash != chash){
        throw TemplateFunctionError("HashMap::copy constructor: both specified and different");
    }
    if (hash == to_copy.hash){
        used = to_copy.used;
        map = copy_hash_table(to_copy.map, to_copy.bins);
    }
    else{
        bins = int(to_copy.size());
        map = new LN* [bins];
        for (int i = 0; i < bins; i++){
            map[i] = new LN();
        }
        for (int i = 0; i < to_copy.bins; i++){
            LN* temp = to_copy.map[i];
            while (temp -> next != nullptr){
                put (temp -> value.first, temp -> value.second);
                temp = temp -> next;
            }
        }
    }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(const std::initializer_list<Entry>& il, double the_load_threshold, int (*chash)(const KEY& k))
:   hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold), bins(il.size())
{
    if (hash == nullptr){
        throw TemplateFunctionError("HashMap::initializer_list constructor : neither specified");
    }
    if (thash != nullptr and chash != nullptr and thash != chash){
        throw TemplateFunctionError("HashMap::initializer_list constructor: both specified and different");
    }
    map = new LN* [bins];
    for (int i = 0; i < bins; i ++){
        map[i] = new LN();
    }
    for (const Entry& m_entry : il){
        put(m_entry.first, m_entry.second);
    }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
template <class Iterable>
HashMap<KEY,T,thash>::HashMap(const Iterable& i, double the_load_threshold, int (*chash)(const KEY& k))
:   hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold), bins(i.size())
{
    if (hash == nullptr){
        throw TemplateFunctionError("HashMap::Iterable constructor: neither specified");
    }
    if (thash != nullptr and chash != nullptr and thash != chash){
        throw TemplateFunctionError("HashMap::Iterable constructor: both specified and different");
    }
    map = new LN* [bins];
    for (int i = 0; i < bins; i++){
        map[i] = new LN();
    }
    for (const Entry& m_entry: i){
        put(m_entry.first, m_entry.second);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::empty() const {
    return used == 0;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
int HashMap<KEY,T,thash>::size() const {
    return used;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::has_key (const KEY& key) const {
    return find_key(key) != nullptr;
}

template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::has_value (const T& value) const {
    for (int i = 0; i < bins; i++){
        LN* temp = map[i];
        while (temp -> next != nullptr){
            if (value == temp -> value.second){
                return true;
            }
            temp = temp -> next;
        }
    }
    return false;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
std::string HashMap<KEY,T,thash>::str() const {
    std::string x;
    return x;
};


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class KEY,class T, int (*thash)(const KEY& a)>
T HashMap<KEY,T,thash>::put(const KEY& key, const T& value) {
    LN* temp = find_key(key);
    T xd;
    mod_count++;
    if (temp == nullptr){
        xd = value;
        used++;
        map[hash_compress(key)] = new LN(Entry(key, value), map[hash_compress(key)]);
    }
    else{
        xd = temp -> value.second;
        temp -> value.second = value;
    }


    return xd;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
T HashMap<KEY,T,thash>::erase(const KEY& key) {
    LN* temp = find_key(key);

    if (temp != nullptr) {
        T xd = temp->value.second;
        LN *to_delete = temp->next;
        *temp = *temp->next;
        delete to_delete;
        used--;
        mod_count++;
        return xd;
    }
    else {
        std::ostringstream answer;
        answer << "HashMap::erase: key(" << key << ") not in Map";
        throw KeyError(answer.str());
    }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::clear() {
    for (int i = 0; i < bins; i++){
        LN* temp = map[i];
        while (temp -> next != nullptr){
            LN* to_delete = temp;
            temp = temp -> next;
            delete to_delete;
        }
        map[i] = temp;
    }

    used = 0;
    mod_count++;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
template<class Iterable>
int HashMap<KEY,T,thash>::put_all(const Iterable& i) {
    int count = 0;
    for (Entry& m_entry : i){
        count++;
        put(m_entry.first, m_entry.second);
    }
    return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators
template<class KEY,class T, int (*thash)(const KEY& a)>
T& HashMap<KEY,T,thash>::operator [] (const KEY& key) {
    int bin = hash_compress(key);
    LN* temp = find_key(key);
    if (temp != nullptr){
        return temp -> value.second;
    }
    map[bin] = new LN(Entry(key, T()), map[bin]);
    used ++;
    mod_count++;
    return map[bin] -> value.second;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
const T& HashMap<KEY,T,thash>::operator [] (const KEY& key) const {
    int bin = hash_compress(key);
    LN* temp = find_key(key);
    if (temp == nullptr){
        throw KeyError("");
    }
    else{
        return temp -> value.second;
    }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>& HashMap<KEY,T,thash>::operator = (const HashMap<KEY,T,thash>& rhs) {
    if (this == &rhs){
        return *this;
    }
    if (hash == rhs.hash and size() == rhs.size()){
        mod_count++;
        map  = copy_hash_table(rhs.map, rhs.bins);
    }else {
        mod_count++;
        clear();
        for (int i = 0; i < rhs.bins; i++) {
            LN *temp = rhs.map[i];
            while (temp->next != nullptr) {
                put(temp->value.first, temp->value.second);
                temp = temp->next;
            }
        }
    }

    return *this;

}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::operator == (const HashMap<KEY,T,thash>& rhs) const {
    if (this == &rhs){
        return true;
    }
    if (used != rhs.size()){
        return false;
    }

    for (int i = 0; i < bins ; i++){
        LN* temp = map[i];
        while (temp -> next != nullptr){
            if (temp -> value.second != rhs[temp -> value.first]){
                return false;
            }
            temp = temp -> next;
        }
    }
    return true;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::operator != (const HashMap<KEY,T,thash>& rhs) const {
    return !(*this == rhs);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
std::ostream& operator << (std::ostream& outs, const HashMap<KEY,T,thash>& m) {
    outs << "map[";

    outs << "]";
    return outs;
}



////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class KEY,class T, int (*thash)(const KEY& a)>
auto HashMap<KEY,T,thash>::begin () const -> HashMap<KEY,T,thash>::Iterator {
    return Iterator(const_cast<HashMap<KEY, T, thash>*>(this), true);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
auto HashMap<KEY,T,thash>::end () const -> HashMap<KEY,T,thash>::Iterator {
    return Iterator(const_cast<HashMap<KEY, T, thash>*>(this), false);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class KEY,class T, int (*thash)(const KEY& a)>
int HashMap<KEY,T,thash>::hash_compress (const KEY& key) const {
    return abs(hash(key)) % bins;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
typename HashMap<KEY,T,thash>::LN* HashMap<KEY,T,thash>::find_key (const KEY& key) const {
    for (LN* temp = map[hash_compress(key)]; temp -> next != nullptr; temp = temp -> next){
        if (key == temp -> value.first){
            return temp;
        }
    }
    return nullptr;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
typename HashMap<KEY,T,thash>::LN* HashMap<KEY,T,thash>::copy_list (LN* l) const {
    if (l -> next == nullptr){
        return new LN();
    }
    LN* to_return = new LN(l -> value, new LN());
    LN* temp = l;
    temp = temp -> next;
    while (temp -> next != nullptr){
        to_return = new LN(temp -> value, to_return);
        temp = temp -> next;
    }
    return to_return;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
typename HashMap<KEY,T,thash>::LN** HashMap<KEY,T,thash>::copy_hash_table (LN** ht, int bins) const {
    LN ** to_return = new LN* [bins];
    for (int i = 0; i < bins; i++){
        to_return[i] = copy_list(ht[i]);
    }
    return to_return;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::ensure_load_threshold(int new_used) {

    if (double(new_used) / double(bins) > load_threshold){
        LN** temp_map = map;
        int temp_bins = bins;

        map = new LN*[temp_bins * 2];
        bins = temp_bins * 2;

        for (int i = 0; i < bins; i++){
            map[i] = new LN();
        }

        for (int i = 0; i < temp_bins; i++){
            LN* temp = temp_map[i];
            while (temp -> next != nullptr){
                int xd = hash_compress(temp -> value.first);
                LN* hehe = temp;
                temp = temp -> next;
                hehe -> next = map[xd];
                map[xd] = hehe;
            }
            delete temp;
        }

    }
    else{
        return;
    }

}


template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::delete_hash_table (LN**& ht, int bins) {
    for (int i = 0; i < bins; i++){
        LN* temp = ht[i];
        while (temp != nullptr){
            LN* to_delete = temp;
            temp = temp -> next;
            delete to_delete;
        }
    }
    delete[] ht;
    ht = nullptr;
}






////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::Iterator::advance_cursors(){
    if (current.second != nullptr and current.second -> next != nullptr and current.second -> next -> next != nullptr){
        current.second = current.second -> next;
        return;
    }
    else {
        for (int i = current.first + 1; i < ref_map->bins; i++) {
            if (ref_map -> map[i] -> next != nullptr) {
                current.first = i;
                current.second = ref_map->map[i];
                return;
            }
        }
    }
    current.first = -1;
    current.second = nullptr;
}
//xd
template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::Iterator::Iterator(HashMap<KEY,T,thash>* iterate_over, bool from_begin)
        : ref_map(iterate_over), expected_mod_count(ref_map->mod_count) {
    current = Cursor(-1, nullptr);
    if(from_begin){
        advance_cursors();
    }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::Iterator::~Iterator()
{}

//xd
template<class KEY,class T, int (*thash)(const KEY& a)>
auto HashMap<KEY,T,thash>::Iterator::erase() -> Entry {
    if (expected_mod_count != ref_map -> mod_count){
        throw ConcurrentModificationError("HashMap::Iterator::erase");
    }
    if (!can_erase){
        throw CannotEraseError("HashMap::Iterator::erase Iterator cursor already erased");
    }
    if (current.second == nullptr){
        throw CannotEraseError("HashMap::Iterator::erase Iterator cursor beyond data structure");
    }

    Entry to_return = current.second -> value;

    return to_return;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
std::string HashMap<KEY,T,thash>::Iterator::str() const {
    std::ostringstream answer;
    answer << ref_map -> str() << "(current=" << current.first << ",expected_mod_count=" << expected_mod_count << ",can_erase" << can_erase << ")";
    return  answer.str();
}

template<class KEY,class T, int (*thash)(const KEY& a)>
auto  HashMap<KEY,T,thash>::Iterator::operator ++ () -> HashMap<KEY,T,thash>::Iterator& {
    if (expected_mod_count != ref_map -> mod_count){
        throw ConcurrentModificationError("HashMap::Iterator::operator ++");
    }
    if (current.second == nullptr){
        return *this;
    }
    if (can_erase or current.second -> next == nullptr){
        advance_cursors();
    }
    can_erase = true;
    return *this;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
auto  HashMap<KEY,T,thash>::Iterator::operator ++ (int) -> HashMap<KEY,T,thash>::Iterator {
    if (expected_mod_count != ref_map -> mod_count){
        throw ConcurrentModificationError("HashMap::Iterator::operator ++(int)");
    }
    if (current.second == nullptr){
        return *this;
    }
    Iterator to_return(*this);
    if (can_erase or current.second -> next == nullptr){
        advance_cursors();
    }
    can_erase = true;
    return to_return;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::Iterator::operator == (const HashMap<KEY,T,thash>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if (rhsASI == 0){
        throw IteratorTypeError("HashMap::Iterator::operator ==");
    }
    if (expected_mod_count != ref_map -> mod_count){
        throw ConcurrentModificationError("HashMap::Iterator::operator ==");
    }
    if(ref_map != rhsASI -> ref_map){
        throw ComparingDifferentIteratorsError("HashMap::Iterator::operator ==");
    }
    return this -> current.second == rhsASI -> current.second;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::Iterator::operator != (const HashMap<KEY,T,thash>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if (rhsASI == 0){
        throw IteratorTypeError("HashMap::Iterator::operator !=");
    }
    if (expected_mod_count != ref_map -> mod_count){
        throw ConcurrentModificationError("HashMap::Iterator::operator !=");
    }
    if (ref_map != rhsASI -> ref_map){
        throw ComparingDifferentIteratorsError("HashMap::Iterator::operator !=");
    }
    return this -> current.second != rhsASI -> current.second;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
pair<KEY,T>& HashMap<KEY,T,thash>::Iterator::operator *() const {
    if (expected_mod_count != ref_map -> mod_count){
        throw ConcurrentModificationError("HashMap::Iterator::operator *");
    }
    if (!can_erase or current.second == nullptr){
        throw IteratorPositionIllegal("HashMap::Iterator::operator * Iterator illegal: exhausted");
    }
    return current.second -> value;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
pair<KEY,T>* HashMap<KEY,T,thash>::Iterator::operator ->() const {
    if (expected_mod_count != ref_map -> mod_count){
        throw ConcurrentModificationError ("HashMap::Iterator::operator *");
    }
    if (!can_erase or current.second == nullptr){
        throw IteratorPositionIllegal("HashMap::Iterator::opeartor -> Iterator illegal: exhausted");
    }
    return &(current.second -> value);
}
}
#endif /* HASH_MAP_HPP_ */
