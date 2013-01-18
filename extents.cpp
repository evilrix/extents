/* vim: set ts=3 sw=3 tw=0 sts=3 et:*/
/**
 * @file extents.cpp
 * @brief Programming challenge
 * @author Ricky Cormier
 * @date 2013-01-17
 *
 * Please note, this is all in one file because that is what the spec
 * called for. This is NOT how I would normally structure a project =/
 *
 * Also note, I have gone waaaaay verbose with the comments just so that my
 * approach is clearly understood. I would not normally pollute code with so
 * many comments (only comment what is necessary since code should be self
 * describing).
 *
 * The following assumptions have been made based upon the provided spec:
 *
 *    a) all input will be valid (so minimal validation is performed)
 *    b) all numbers can be represented using an unsigned integer
 */

// ---------------------------------------------------------------------------
// required headers
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <utility>
#include <functional>
#include <iomanip>
#include <iterator>
#include <stdexcept>
#include <sstream>
#include <string>
#include <memory>
#include <cassert>
#include <cstring>

// ---------------------------------------------------------------------------
// Nothing to see here, just a little namespace injection!
namespace std
{
   // Inject a streaming operator for std::pair<F,S>
   template <typename F, typename S>
      istream & operator >> (istream & in, pair<F, S> & o)
      {
         return in >> o.first >> o.second;
      }
}

// ---------------------------------------------------------------------------
// save on a little typing (this should NEVER be done in a header file!
using namespace std;

// not my most creative namespace name!
namespace challenge
{
   // since we're not able to use boost::bind, we'll use a little template
   // magic to allow us to define simple predicates that either operate on
   // the first or the second of a pair, depending upon the template param.

   // this is the generic version, which we need to specialise for 1 and 2
   template <int>
      struct which;

   // specialisation for the 'first' member of a pair
   template <>
      struct which<1>
      {
         // we only need a less so that's all we're implementing
         template <typename F, typename S>
            static bool less(pair<F, S> const & lhs, pair<F, S> const & rhs)
            {
               return lhs.first < rhs.first;
            }
      };

   // specialisation for the 'second' member of a pair
   template <>
      struct which<2>
      {
         // we only need a less so that's all we're implementing
         template <typename F, typename S>
            static bool less(pair<F, S> const & lhs, pair<F, S> const & rhs)
            {
               return lhs.second < rhs.second;
            }
      };

   // The extents class represents the extents dataset. It transforms the raw
   // dataset into a vector of pairs, each of which represents either the start
   // or end of a range and the number of ranges that overlap that range. Put
   // another way, we are building a data structure that represents this:
   //
   // This is an example data set of extents
   //    0 40
   //    2 12
   //    4 30
   //    6 21
   //    24 30
   //
   // When stacked on top of each other this is how they'd look.
   //
   //           1         2         3
   // 0123456789012345678901234567890123456789
   // ----------------------------------------
   // 1 2 3 4     3        2  3     1   0      <--- count of overlapped extents
   // ----------------------------------------
   //
   //   |---------|
   //       |--------------|
   //     |-------------------------|
   // |---------------------------------|
   //                         |-----|
   //
   // It should be obvious that to see how many ranges a value falls within all
   // one must do is count the number of lines that overlap. We are going to
   // create a nice simple data structure that allows us to do just that.
   //
   // Firstly, we need to load the data and flag the start and end of ranges. To
   // do that we're going to use 1 and -1 and the reason why is because once we
   // sort the pairs we can just iterate the resultant sorted vector and add up
   // each of the values. This will result in the addition of subtraction of an
   // overlap count.
   //
   // This is how the data is originally loaded.
   //    0(1) 2(1) 4(1) 6(1) 12(-1) 21(-1) 24(1) 30(-2) 40(-1)
   //
   // The data is then transformed to include the accumulated range overlaps
   //    0(1) 2(2) 4(3) 6(4) 12(3) 21(2) 24(3) 30(1) 40(0)
   //
   // Now, to find how many extents number falls within we just binary chop the
   // vector (using upper_bounds) and the marker at the previous position will
   // contain the count of the number of overlaps, thus, the number of extents
   // in which that number falls.
   //
   // This does mean there is a bit of processing needed to create the structure
   // but since the number of extents vs. the number of numbers we are going to
   // look up is very one sides it makes sense to do a little pre-processing to
   // generate a data structure that gives us the best possible time complexity.
   //
   // The only possible concern with a binary search is that it's not the most
   // hardware efficient lookup algorithm due to page faults/memory cache being
   // invalidated; however, for such a small number of elements in the vector
   // I'd expect the OS to be able to hold the whole vector in a single page,
   // thus page fault/ cache invalidation issues should not be a concern.
   //
   // Is there a better way to do this? Almost definately; however, I feel that
   // this solution finds a nice balance between complexity and efficiency. The
   // rule I use is always strive for an optimal solution but don't worry about
   // optimizing unless (a) it is proven there is a performance problem and (b)
   // you use a profiler to identify exactly what is causing that problem.

   class extents
   {
      public:
         // construct-o-tron (tm)
         extents() { }
         extents(istream & in) { init(in); }
         extents(istream & in, size_t const reserve) { init(in); }

         // set stuff up
         void init(istream & in,
               size_t const reserve = 50000 // max extent items expected
               )
         {
            data_.reserve(reserve); // just to make push_back more efficient

            // Read the extents file into a vector of pairs, first will be the
            // range marker (start & end) and second will be a 1 to indicate the
            // start of a range and -1 (actually, since it's unsigned it'll be a
            // binary representation of -1 rather than real decimal -1) will be
            // used to indicate end of a range. We'll, later, accumulate these
            // to indicate the number of overlapped extents for any point in the
            // overall extent range.
            in.exceptions(ios::badbit);
            pair<unsigned, unsigned> x;

            while(in >> x)
            {
               // start of an extent
               data_.push_back(make_pair(x.first, 1));
               //end of an extent
               data_.push_back(make_pair(x.second, -1));
            }

            // sort based on the first element, creating an extent range vector
            sort(data_.begin(), data_.end(),
                  which<1>::less<unsigned, unsigned>);

            // Now we just need to iterate the vector and accumulate the overlap
            // counts for each extent marker, which will give us a simple vector
            // of extent ranges, with the number of overlaps in a nice easy to
            // search (using upper_bound) data structure.
            unsigned cnt = 0;
            vector<pair<unsigned, unsigned> >::iterator iitr = data_.begin();
            vector<pair<unsigned, unsigned> >::iterator oitr = data_.begin();

            // interate range vector, accumulate overlaps and compact by
            // removing any unnecessary duplicate range markers.
            for(;;)
            {
               // accumulate
               cnt += iitr->second;
               *oitr = make_pair(iitr->first, cnt);

               // end of vector, good stuff - let's move on
               if(++iitr == data_.end()) { break; }

               // The orignal vector might contain duplicate values (as a range
               // may start or end at same place multiple times. We'll remove
               // the dups but still accumulate the overlap counts.
               if(iitr->first != oitr->first) { ++oitr; }
            }

            // In the previous stage we compacted the vector by removing dups.
            // If we had dups we need to shrink vector as we've compressed them
            // out and so the remainder of the vector is junk. Resize is a nice
            // way to do this as it has a O(1) complexity.
            data_.resize(++oitr - data_.begin());

            // At this point we have a vector that probably has excess memory
            // allocated to it. Given that the spec states 50,000 items is the
            // max it's hardly worth worrying about but if the spec were to
            // suggest other wise it would be worth trimming the excess.
            // Unfortunately, C++ doesn't provide an official way to do this but
            // there is a trick called the "vector swap trick" that will do just
            // that. It involves creating a new temporary vector, copying old
            // to the temporary (such that temporary will be perfectly sized)
            // and then swapping temporarie's internal buffer (just a pointer
            // swap) for the current vector's. Excess buffer is then returned
            // to the heap manager for reallocation.
            //
            // http://bit.ly/Xk9Tnz
            //
            // It goes a little something like this...
            //    vector<pair<unsigned, unsigned> >(data_).swap(data_);
         }

         // tries to find the number of extents into which n might fit
         unsigned find(unsigned const n)
         {
            // We have a sorted vector of ranges so we use upper_bound to see
            // if n falls within any of the ranges. If we find a match we'll be
            // able to get the number of range overlaps and return it.
            unsigned cnt = 0;

            vector<pair<unsigned, unsigned> >::const_iterator itr =
               lower_bound(data_.begin(), data_.end(), make_pair(n, 0),
                     which<1>::less<unsigned, unsigned>);

            // Does n fall within our extent range?
            if(itr != data_.end())
            {
               if(n < itr->first)
               {
                  // Found mid-range value we just return the overlap count for
                  // this range (which is actually denoated by the previous
                  // range marker, since the upperbound is the end of the range.
                  cnt = (itr-1)->second;
               }
               else
                  if(n == itr->first)
                  {
                     // If we find an exact match we need to identify if it's
                     // the start or end of a range (except when we're at
                     // position 0 since we know that must be the start). If
                     // the previous marker's count is greater than the current
                     // we're at the end of a range so use the pervious marker
                     // to obtain the count value, otherwise we're at the start
                     // of a range so we can use the current markers count
                     // value.
                     if(itr != data_.begin() && (itr-1)->second > itr->second)
                     {
                        cnt = (itr-1)->second;
                     }
                     else
                     {
                        cnt = itr->second;
                     }
                  }
            }

            return cnt; // return the range count.
         }

      private:
         // This vector will contain a list of our extent markers and the number
         // of overlapping ranges that each marker pair represents. This allows
         // for a lookup complexity of O(log N). Not quite as good as O(1) but
         // still way better than the O(N) a naive solution would likely impose.
         // Overall our runtime complexity will be O(N log N), where N is how
         // many numbers we need to process. If we used a naive solution the
         // runtime complexity would be approcimately quadratic O(N^2) - eeek!
         // NB. All expressed time complexities are amortised.
         vector<pair<unsigned, unsigned> > data_;
   };

}

// ---------------------------------------------------------------------------
// a quick and dirty bit of testing (I'd normally use a proper testing
// framework (such as Boost Unit Tesing Framework) but since the spec requires
// nothing but standard C++ I've cobble together something.

// a unique exception type we can use within our fixture
struct utexcept : runtime_error
{
   utexcept(string const & msg)
      : runtime_error(msg) {}
};

// a few macros do define our text fixture
#define START_TEST(name) \
   try {

#define END_TEST() \
   } \
   catch (utexcept const & e) \
      { std::cerr << "TEST FAILURE: " << e.what() << "\n"; } \
   catch (exception const & e) \
      { std::cerr << "TEST ERROR: " << e.what() << "\n"; } \
   catch (...) \
      { std::cerr << "TEST ERROR: cause unknown\n"; } \

// a few macros to perform testing
#define UTASSERT_(pred, msg, line, file) \
   { \
      if(! (pred) ) \
      { \
         ostringstream oss; \
         oss << file << "(" << line << "): " << msg << "\n"; \
         throw utexcept(oss.str()); \
      } \
   }

#define UTASSERT_MSG(pred, msg) \
   UTASSERT_(pred, msg, __LINE__, __FILE__);

#define UTASSERT(pred) \
   UTASSERT_MSG(pred, #pred);

#define UTASSERT_EQUAL(lhs, rhs) \
   UTASSERT_MSG(lhs==rhs, "equality expected");

// ---------------------------------------------------------------------------
// I prefer to encapsulate all my "app" functionality into a little singleton
// class for two main reasons:
//
//   a) it's a little more OO like to have an object as the entry-point
//   b) it means that main()'s only task is to handle uncaught exceptions
struct app
{
   // Just a simple self test, which can be invoked from the command line.
   // The "expected" data should be a text stream piped to stdin.
   static void self_test(unsigned const cnt)
   {
      START_TEST("self test")

      unsigned in;
      cin >> in;

      UTASSERT(cin.good())
      UTASSERT_EQUAL(in, cnt)

      END_TEST()
   }

   // this is the app entrypoint
   static void run(bool const test)
   {
      // we need a little input (note, names are hard coded for convenience
      // because the spec asserts this is what they must be called.
      ifstream in_ext("extents.txt");
      if(!in_ext) { throw runtime_error("unable to open extents.txt file"); }
      in_ext.exceptions(ios::badbit);

      ifstream in_num("numbers.txt");
      if(!in_ext) { throw runtime_error("unable to open numbers.txt file"); }
      in_num.exceptions(ios::badbit);

      // create an extents lookup object and then process the numbers
      challenge::extents extents(in_ext);
      unsigned num = 0;
      while(in_num >> num)
      {
         unsigned const cnt = extents.find(num);

         // to test or not to test, that is the question!
         if(test)
         {
            // seems we're being slightly noble here
            self_test(cnt);
         }
         else
         {
            // meh, I don't need no testing - just do it
            cout << cnt << endl;
         }

      }
   }
};

// ---------------------------------------------------------------------------
int main (int argc, char * argv[])
{
   int retval = 0;

   try
   {
      // main entry point for application
      app::run(argc > 1 && (0 == strcmp(argv[1], "test")));
   }
   catch(std::exception const & ex)
   {
      std::cerr << ex.what() << "\n";
      retval = 1;
   }
   catch(...)
   {
      std::cerr << "Unknown exception\n";
      retval = 2;
   }

   return retval;
}

