/* vim: set ts=3 sw=3 tw=0 sts=3 et:*/
/**
 * @file extents.cpp
 * @brief Blismedia programming challenge
 * @author Ricky Cormier
 * @date 2013-01-17
 */

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
#include <cassert>

namespace std // Nothing to see here, just a little namespace injection!
{
   // Inject a streaming operator for std::pair<F,S> into the std
   // namespace so that we can use std::copy with istream_iterator
   template <typename F, typename S>
      istream & operator >> (istream & in, pair<F, S> & o)
      {
         return in >> o.first >> o.second;
      }
}

using namespace std;

namespace blismedia
{
   template <int>
      struct which;

   template <>
      struct which<1>
      {
         template <typename F, typename S>
            static bool less(pair<F, S> const & lhs, pair<F, S> const & rhs)
            {
               return lhs.first < rhs.first;
            }
      };

   template <>
      struct which<2>
      {
         template <typename F, typename S>
            static bool less(pair<F, S> const & lhs, pair<F, S> const & rhs)
            {
               return lhs.second < rhs.second;
            }
      };

   class extents
   {
      public:
         extents() { }
         extents(istream & in) { init(in); }
         extents(istream & in, size_t const reserve) { init(in); }

         void init(istream & in, size_t const reserve = 50000)
         {
            in.exceptions(ios::badbit);
            pair<unsigned, unsigned> x;

            while(in >> x)
            {
               data_.push_back(make_pair(x.first, 1));
               data_.push_back(make_pair(x.second, -1));
            }

            sort(data_.begin(), data_.end(), which<1>::less<unsigned, unsigned>);

            unsigned cnt = 0;
            vector<pair<unsigned, unsigned> >::iterator iitr = data_.begin();
            vector<pair<unsigned, unsigned> >::iterator oitr = data_.begin();

            for(;;)
            {
               cnt += iitr->second;
               *oitr = make_pair(iitr->first, cnt);

               if(++iitr == data_.end()) { break; }

               if(iitr->first != oitr->first)
               {
                  ++oitr;
               }
            }

            data_.resize(++oitr - data_.begin());
         }

         unsigned find(unsigned const n)
         {
            unsigned cnt = 0;

            vector<pair<unsigned, unsigned> >::const_iterator itr =
               lower_bound(data_.begin(), data_.end(), make_pair(n, 0), which<1>::less<unsigned, unsigned>);

            if(itr != data_.end())
            {
               if(n < itr->first)
               {
                  cnt = (itr-1)->second;
               }
               else
                  if(n == itr->first)
                  {
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

            return cnt;
         }

      private:
         vector<pair<unsigned, unsigned> > data_;
   };

}

struct app
{
   static void run()
   {
      std::ifstream in_ext("extents.txt");

      if(!in_ext) { throw runtime_error("unable to open extents.txt file"); }

      std::ifstream in_exp("expected.txt");
      if(!in_ext) { throw runtime_error("unable to open expected.txt file"); }

      std::ifstream in_num("numbers.txt");
      if(!in_ext) { throw runtime_error("unable to open numbers.txt file"); }

      blismedia::extents extents(in_ext);

      assert(extents.find(102731) == 97270);
      assert(extents.find(102544) == 97457);
      assert(extents.find(108760) == 91241);
   }
};

int main ()
{
   int retval = 0;

   try
   {
      app::run();
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

