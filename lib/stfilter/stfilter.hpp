// +-------------------------------------------------------------------------+
// |                   StreamFilters library vers. 0.1.01                    |
// |  Copyright (c) Andrey V. Stolyarov <croco at croco dot net> 2022, 2023  |
// | ----------------------------------------------------------------------- |
// | This is free software.  Permission is granted to everyone to use, copy  |
// |        or modify this software under the terms and conditions of        |
// |                 GNU LESSER GENERAL PUBLIC LICENSE, v. 2.1               |
// |     as published by Free Software Foundation (see the file LGPL.txt)    |
// |                                                                         |
// | Please visit http://www.croco.net/software/stfilter to get a fresh copy |
// | ----------------------------------------------------------------------- |
// |   This code is provided strictly and exclusively on the "AS IS" basis.  |
// | !!! THERE IS NO WARRANTY OF ANY KIND, NEITHER EXPRESSED NOR IMPLIED !!! |
// +-------------------------------------------------------------------------+




#ifndef STFILTER_HPP_SENTRY
#define STFILTER_HPP_SENTRY


//! Abstract filter for a char stream
/*! This class represents a generic filter that accepts a text,
    chars by char, does some transformations and passes the result
    on to the next filter in a chain.
    It is assumed the last item in the chain either stores the chars,
    or may be does something else, e.g. outputs them.
 */
class StreamFilter {
    StreamFilter *dest;
public:
    StreamFilter(StreamFilter *dst) : dest(dst) {}
    virtual ~StreamFilter() {}
    virtual void FeedChar(int c) = 0;
    virtual void FeedEnd() { PutEnd(); }

    void ChainReset(); // calls Reset() for every object in the chain
    virtual void Reset() {}

        //! Add an object to the end of the chain
        /*! Normally the chain is constructed in the reverse order,
            so every object is given the address of its next one
            right in the constructor.  However, in some cases it is
            convenient to add filters in the 'native' order, from
            left to right; this method does it.
            \warning Don't use it for lenghty chains!  The chain is
            traversed every time, so you've got O(n^2) operations.
            For small chains it is okay, but may be too slow for more
            items in the chain.
         */
    void AddToEnd(StreamFilter *p);

        //! Add an object to the chain right after this one
    void InsertAfter(StreamFilter *p);

        //! delete all objects in this chain, including *this
        /*! \note Normally StremFilter objects don't own the
            objects pointed to by dest; this allows to allocate
            all objects on the stack (as local variables). If,
            however, all the objects are created on the heap
            (with the ``new'' operator), it is convenient to be
            able to delete them all at once.
            \warning Never use this method unless you're sure
            all objects in the chain are allocated with ``new''.
         */
    void DeleteChain();
protected:
    void PutChar(int c) { if(dest) dest->FeedChar(c); }
    void PutEnd() { if(dest) dest->FeedEnd(); }
    void PutStr(const char *s);

        /*! \param minsigns minimal number of digits (zeroes added
                   as needed); 0 means no padding
            \param uppercase if true, 'A'..'Z' are used,
                   'a'..'z' otherwise
         */
    void PutHex(unsigned long long n, int minsigns, bool uppercase);

    StreamFilter *GetNext() const { return dest; }
};

//! The dumb filter object, which accumulates the received stream
/*! Objects of this class are intended to serve as the last item
    in the filter chain.  All received chars are stored in
    a string buffer inside the object, which can later be retieved.
    The string is always zero-terminated.
 */
class StreamFilterDestination : public StreamFilter {
    enum { start_bufsize = 32, bufsize_step = 32 };
        /* bufsize_step == 0  means the size is doubled every time */

    char *buf;
    int buflen, buf_used;
public:
    StreamFilterDestination();
    ~StreamFilterDestination();

        //! return the buffer's address and forget the buffer
        /*! \warning this can return 0 in case no chars are
            received so far
         */
    char *ReleaseBuffer();
private:
    virtual void FeedChar(int c);
    virtual void Reset();
    void More();
};


#endif
