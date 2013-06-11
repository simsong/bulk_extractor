#ifndef _RAR_ARRAY_
#define _RAR_ARRAY_

#include "rardefs.hpp"
#include "errhnd.hpp"

extern ErrorHandler ErrHandler;

template <class T> class Array
{
  private:
    T *Buffer;
    size_t BufSize;
    size_t AllocSize;
  public:
    Array();
    Array(size_t Size);
    Array(const Array<T> &copy);
    ~Array();
    inline void CleanData();
    inline T& operator [](size_t Item);
    inline size_t Size(); // Returns the size in items, not in bytes.
    void Add(size_t Items);
    void Alloc(size_t Items);
    void Reset();
    const Array<T>& operator=(const Array<T> &Src);
    void Push(T Item);
    T* Addr() {return(Buffer);}
};

template <class T> void Array<T>::CleanData()
{
  Buffer=NULL;
  BufSize=0;
  AllocSize=0;
}


template <class T> Array<T>::Array() :
    Buffer(), BufSize(), AllocSize()
{
  CleanData();
}


template <class T> Array<T>::Array(size_t Size_) :
    Buffer(), BufSize(), AllocSize()
{
  Buffer=(T *)malloc(sizeof(T)*Size_);
  if (Buffer==NULL && Size_!=0)
    ErrHandler.MemoryError();

  AllocSize=BufSize=Size_;
}

template <class T> Array<T>::Array(const Array<T> &copy)
{
    *this = copy;
}


template <class T> Array<T>::~Array()
{
  if (Buffer!=NULL)
    free(Buffer);
}


template <class T> inline T& Array<T>::operator [](size_t Item)
{
  return(Buffer[Item]);
}


template <class T> inline size_t Array<T>::Size()
{
  return(BufSize);
}


template <class T> void Array<T>::Add(size_t Items)
{
  BufSize+=Items;
  if (BufSize>AllocSize)
  {
    size_t Suggested=AllocSize+AllocSize/4+32;
    size_t NewSize=Max(BufSize,Suggested);

    Buffer=(T *)realloc(Buffer,NewSize*sizeof(T));
    if (Buffer==NULL)
      ErrHandler.MemoryError();
    AllocSize=NewSize;
  }
}


template <class T> void Array<T>::Alloc(size_t Items)
{
  if (Items>AllocSize)
    Add(Items-BufSize);
  else
    BufSize=Items;
}


template <class T> void Array<T>::Reset()
{
  if (Buffer!=NULL)
  {
    free(Buffer);
    Buffer=NULL;
  }
  BufSize=0;
  AllocSize=0;
}


template <class T> const Array<T>& Array<T>::operator=(const Array<T> &Src)
{
  Reset();
  Alloc(Src.BufSize);
  if (Src.BufSize!=0)
    memcpy((void *)Buffer,(void *)Src.Buffer,Src.BufSize*sizeof(T));
  return *this;
}


template <class T> void Array<T>::Push(T Item)
{
  Add(1);
  (*this)[Size()-1]=Item;
}

#endif
