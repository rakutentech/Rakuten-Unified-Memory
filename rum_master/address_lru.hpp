#ifndef ADDRESS_LRU_
#define ADDRESS_LRU_

#include <cstdint>
#include <cstddef>

#include <list>
#include <mutex>

/*
 * Note, in this class stale is refferred to as being pushed out of the FIFO
 */
class AddressLRU {
public:
  typedef uint64_t Addr;

  AddressLRU();
  AddressLRU(const AddressLRU&);
  ~AddressLRU();

  /**
   * Initialize
   * 
   * \param size Cache size
   * \return Nothing
   */
  void init(size_t size = 10);

  /**
   *  Does the cache contain 'address'?
   * 
   * \param address the address to look for
   * \return true if the address is in the cache, otherwise false
   */
  bool contains(Addr address);

  /**
   * Thread SAFE
   * 
   * \param address Address to add
   * \return stale_address value or 0 if nothing ohas become stale
   */
  Addr update(Addr address);

  /**
   * Clear history.
   * 
   * \return None
   */
  void clear(void);
  
  /**
   * Report current size
   */
  size_t size(void) const;

  /**
   * Report statistics
   * 
   * \return None
   */
  void reportStats(void) const;

  /**
   * Remove any values in the cache if they are between the start and end range
   *
   * \param start address of the start of the range 
   * \param end   address of the end of the range
   */
  void purge(Addr start, Addr end);

  const std::list<Addr> get_list() {return history_;}
  
  void dump();

protected:
  size_t size_limit_;
  std::list< Addr > history_; // newest last

  size_t count_access_;
  size_t count_stale_;

private:
  static size_t count_; // instance count
  std::mutex cache_mutex;
};

#endif
