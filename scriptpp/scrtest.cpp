// +-------------------------------------------------------------------------+
// |                     Script Plus Plus vers. 0.3.70                       |
// | Copyright (c) Andrey V. Stolyarov  <croco at croco dot net>  2003--2023 |
// | ----------------------------------------------------------------------- |
// | This is free software.  Permission is granted to everyone to use, copy  |
// |        or modify this software under the terms and conditions of        |
// |                 GNU LESSER GENERAL PUBLIC LICENSE, v. 2.1               |
// |     as published by Free Software Foundation (see the file LGPL.txt)    |
// |                                                                         |
// | Please visit http://www.croco.net/software/scriptpp to get a fresh copy |
// | ----------------------------------------------------------------------- |
// |   This code is provided strictly and exclusively on the "AS IS" basis.  |
// | !!! THERE IS NO WARRANTY OF ANY KIND, NEITHER EXPRESSED NOR IMPLIED !!! |
// +-------------------------------------------------------------------------+




#include <stdio.h>
#include <unistd.h>
#include "tests.h"

#include "scrvar.hpp"
#include "scrvect.hpp"
#include "scrmap.hpp"
#include "scrsubs.hpp"
#include "scrmacro.hpp"
#include "conffile.hpp"
#include "confinfo.hpp"
#include "scrmsg.hpp"

class PrefReq : public ScriptSubstitutionPrefixRequest {
public:
    PrefReq() : ScriptSubstitutionPrefixRequest("req") {}
    ScriptVariable Handle(const ScriptVector &v) const {
        ScriptVariable res;
        for(int i = 0; i < v.Length(); i++) {
            res += '(';
            res += v[i];
            res += ')';
        }
        return res;
    }
};

class MacroFunc : public ScriptMacroprocessorMacro {
public:
    MacroFunc(const char *nm = "req", bool dirty = false)
        : ScriptMacroprocessorMacro("req") {}
    ScriptVariable Expand(const ScriptVector &v) const {
        ScriptVariable res;
        for(int i = 0; i < v.Length(); i++) {
            res += '(';
            res += v[i];
            res += ')';
        }
        return res;
    }
    ScriptVariable Expand() const { return "<empty>"; }
};
class MacroQuote : public ScriptMacroprocessorMacro {
public:
    MacroQuote() : ScriptMacroprocessorMacro("q") {}
    ScriptVariable Expand(const ScriptVector &v) const {
        return v[0];
    }
    ScriptVariable Expand() const { return "<empty>"; }
};

int main()
{
  try {
      test_suite("ScriptVariable");
      test_subsuite("construction");
      {
         ScriptVariable v1("a");
         test_long("a", v1.length(), 1);
         ScriptVariable v2("");
         test_long("empty", v2.length(), 0);
      }
      test_subsuite("valid_and_invalid");
      {
         ScriptVariable v1("a");
         ScriptVariable v2("");
         ScriptVariable v3(0);
         ScriptVariableInv v4;
         ScriptVariable v5((char*)0);

         test("str_valid", v1.IsValid() && !v1.IsInvalid());
         test("empty_str_valid", v2.IsValid() && !v2.IsInvalid());
         test("zero_invalid", !v3.IsValid() && v3.IsInvalid());
         test("inv_invalid", !v4.IsValid() && v4.IsInvalid());
         test("null_ptr_invalid", !v5.IsValid() && v5.IsInvalid());
      }
      test_subsuite("assignments");
      {
         ScriptVariable v1;
         v1 = "abc";
         test_long("length", v1.length(), 3);
         test_str("c_str", v1.c_str(), "abc");
      }
      test_subsuite("addition");
      {
         ScriptVariable abc("abc");
         ScriptVariable res = abc + '.' + abc + '.' + abc;
	 test_long("added_length", res.length(), 11);
         test_str("added_str", res.c_str(), "abc.abc.abc");
      }
      test_subsuite("substirng");
      {
         ScriptVariable v1;
         v1 = "abcdefgh";
         ScriptVariable::Substring s1(v1, 0, 100);
         test_str("sub1", s1.Get().c_str(), "abcdefgh");
         ScriptVariable::Substring s2(v1, 0, 1);
         test_str("sub2", s2.Get().c_str(), "a");
         ScriptVariable::Substring s3(v1, 0, 3);
         test_str("sub3", s3.Get().c_str(), "abc");
         ScriptVariable::Substring s4(v1, 3, 3);
         test_str("sub4", s4.Get().c_str(), "def");
         ScriptVariable::Substring s5(v1, 3, 10000);
         test_str("sub5", s5.Get().c_str(), "defgh");
         ScriptVariable::Substring s6(v1, -5, 2);
         test_str("negative", s6.Get().c_str(), "de");
         ScriptVariable::Substring s7(v1, 0, 0);
         test_str("empty", s7.Get().c_str(), "");
         test("empty_eq", s7.Get() == ScriptVariable(""));
         v1.Range(-1,1).Erase();
         test_str("remove_last_char", v1.c_str(), "abcdefg");
         ScriptVariable v2;
         v2 = "abcdefgh";
         ScriptVariable::Substring(v2, 2, 2).Erase();
         test_str("erase", v2.c_str(), "abefgh");
         ScriptVariable::Substring(v2, 3, 200).Erase();
         test_str("erase_to_end", v2.c_str(), "abe");
         ScriptVariable v3("alpha->beta");
         ScriptVariable::Substring s20 = v3.Strstr("->");
         test_str("substring_find", s20.Get().c_str(), "->");
         test_str("substring_before", s20.Before().Get().c_str(), "alpha");
         test_str("substring_after", s20.After().Get().c_str(), "beta");
      }

      test_subsuite("strchr");
      {
         ScriptVariable v1;
         v1 = "abcdefgeh";
         ScriptVariable::Substring sub;

         sub = v1.Strchr('e');
         test("strchr_valid", sub.IsValid());
         test("strchr_not_invalid", !sub.IsInvalid());
         test_str("strchr", sub.Get().c_str(), "e");
         test_long("strchr_left", sub.Index(), 4);

         sub = v1.Strchr('q');
         test("strchr_invalid", sub.IsInvalid());
         test("strchr_not_valid", !sub.IsValid());

         sub = v1.Strrchr('e');
         test("strrchr_valid", sub.IsValid());
         test("strrchr_not_invalid", !sub.IsInvalid());
         test_str("strrchr", sub.Get().c_str(), "e");
         test_long("strrchr_right", sub.Index(), 7);


      }

      test_subsuite("strstr");
      {
         ScriptVariable v1;
         v1 = "abcdefghdefpqr";
         ScriptVariable::Substring sub;

         sub = v1.Strstr("def");
         test("strstr_valid", sub.IsValid());
         test("strstr_not_invalid", !sub.IsInvalid());
         test_str("strstr", sub.Get().c_str(), "def");
         test_long("strstr", sub.Index(), 3);

         sub = v1.Strstr("deg");
         test("strstr_invalid", sub.IsInvalid());
         test("strstr_not_valid", !sub.IsValid());
      }

      test_subsuite("strrstr");
      {
         ScriptVariable v1;
         v1 = "abcdefghdefpqr";
         ScriptVariable::Substring sub;

         sub = v1.Strrstr("def");
         test("strrstr_valid", sub.IsValid());
         test("strrstr_not_invalid", !sub.IsInvalid());
         test_str("strrstr", sub.Get().c_str(), "def");
         test_long("strrstr", sub.Index(), 8);

         sub = v1.Strrstr("deg");
         test("strrstr_invalid", sub.IsInvalid());
         test("strrstr_not_valid", !sub.IsValid());
      }

      test_subsuite("upperlower");
      {
         ScriptVariable v1 = "abcDEFpqrXZY123=-";
         v1.Toupper();
         test_str("toupper", v1.c_str(), "ABCDEFPQRXZY123=-");

         ScriptVariable v2 = "abcDEFpqrXZY123=-";
         v2.Tolower();
         test_str("tolower", v2.c_str(), "abcdefpqrxzy123=-");
      }
      test_subsuite("trim");
      {
         {
         ScriptVariable v1;
         v1 = "       abc  ";
         ScriptVariable::Substring Sub(v1);
         Sub.Trim(" ");
         test_str("trim", Sub.Get().c_str(), "abc");
         }
         {
         ScriptVariable v1;
         v1 = "abc";
         ScriptVariable::Substring Sub(v1);
         Sub.Trim(".");
         test_str("trim_noop", Sub.Get().c_str(), "abc");
         }
         {
         ScriptVariable v1;
         v1 = "a";
         ScriptVariable::Substring Sub(v1);
         Sub.Trim(".");
         test_str("trim_noop", Sub.Get().c_str(), "a");
         }
         {
         ScriptVariable v1;
         v1 = "........";
         ScriptVariable::Substring Sub(v1);
         Sub.Trim(".");
         test_str("trim_all", Sub.Get().c_str(), "");
         }
         {
         ScriptVariable v1;
         v1 = "       abc  ";
         v1.Trim(" ");
         test_str("trim_sv", v1.c_str(), "abc");
         }
      }

      test_subsuite("prefix");
      {
         ScriptVariable v("abcdefgh");
         test("prefix1", v.HasPrefix("a"));
         test("prefix2", v.HasPrefix("ab"));
         test("prefix3", v.HasPrefix("abc"));
         test("prefix4", v.HasPrefix("abcd"));
         test("prefix5", v.HasPrefix("abcde"));
         test("empty_prefix", v.HasPrefix(""));
         test("prefix_self", v.HasPrefix("abcdefgh"));
         test("super_prefix", !v.HasPrefix("abcdefghij"));
         test("prefix_fail", !v.HasPrefix("abcd1"));
         test("prefix_fail2", !v.HasPrefix("12345"));
      }

      test_subsuite("suffix");
      {
         ScriptVariable v("abcdefgh");
         test("suffix1", v.HasSuffix("h"));
         test("suffix2", v.HasSuffix("gh"));
         test("suffix3", v.HasSuffix("fgh"));
         test("suffix4", v.HasSuffix("efgh"));
         test("suffix5", v.HasSuffix("defgh"));
         test("empty_suffix", v.HasSuffix(""));
         test("suffix_self", v.HasSuffix("abcdefgh"));
         test("super_suffix", !v.HasSuffix("pqabcdefgh"));
         test("suffix_fail", !v.HasSuffix("1efgh"));
         test("suffix_fail2", !v.HasSuffix("12345"));
      }

      test_subsuite("suffix_v");
      {
         ScriptVariable v("abcdefgh");
         test("suffix1", v.HasSuffix(ScriptVariable("h")));
         test("suffix2", v.HasSuffix(ScriptVariable("gh")));
         test("suffix3", v.HasSuffix(ScriptVariable("fgh")));
         test("suffix4", v.HasSuffix(ScriptVariable("efgh")));
         test("suffix5", v.HasSuffix(ScriptVariable("defgh")));
         test("empty_suffix", v.HasSuffix(ScriptVariable("")));
         test("suffix_self", v.HasSuffix(ScriptVariable("abcdefgh")));
         test("super_suffix", !v.HasSuffix(ScriptVariable("pqabcdefgh")));
         test("suffix_fail", !v.HasSuffix(ScriptVariable("1efgh")));
         test("suffix_fail2", !v.HasSuffix(ScriptVariable("12345")));
      }

      test_subsuite("fetch words");
      {
         ScriptVariable v1;
         v1 = " abc    def ghijh  q  ";
         ScriptVariable::Substring iter(v1);
         ScriptVariable::Substring word;

         test("first_word", iter.FetchWord(word));
         test_str("first_word_value", word.Get().c_str(), "abc");
         test("second_word", iter.FetchWord(word));
         test_str("second_word_value", word.Get().c_str(), "def");
         test("third_word", iter.FetchWord(word));
         test_str("third_word_value", word.Get().c_str(), "ghijh");
         test("fourth_word", iter.FetchWord(word));
         test_str("fourth_word_value", word.Get().c_str(), "q");
         test("no_more_words", !iter.FetchWord(word));

         ScriptVariable v2;
         v2 = "abc    def ghijh  q  ";
         ScriptVariable::Substring iter2(v2);
         test("first_word_at_begin", iter2.FetchWord(word));
         test_str("first_word_at_begin_value", word.Get().c_str(), "abc");

         ScriptVariable v3;
         v3 = "abc";
         ScriptVariable::Substring iter3(v3);
         test("first_word_the_only", iter3.FetchWord(word));
         test_str("first_word_the_only", word.Get().c_str(), "abc");
         test("first_word_the_only_ends", !iter3.FetchWord(word));

         ScriptVariable v4;
         v4 = "   ";
         ScriptVariable::Substring iter4(v4);
         test("no_words", !iter4.FetchWord(word));

         ScriptVariable v5;
         ScriptVariable::Substring iter5(v5);
         test("empty_no_words", !iter5.FetchWord(word));
      }
      test_subsuite("fetch tokens");
      {
         ScriptVariable v1;
         v1 = "abc,def,ghijh,q,qqqq";
         ScriptVariable::Substring iter(v1);
         ScriptVariable::Substring word;
         test("first_token", iter.FetchToken(word));
         test_str("first_token_value", word.Get().c_str(), "abc");
         test("second_token", iter.FetchToken(word));
         test_str("second_token_value", word.Get().c_str(), "def");
         test("third_token", iter.FetchToken(word));
         test_str("third_token_value", word.Get().c_str(), "ghijh");
         test("fourth_token", iter.FetchToken(word));
         test_str("fourth_token_value", word.Get().c_str(), "q");
         test("fifth_token", iter.FetchToken(word));
         test_str("fifth_token_value", word.Get().c_str(), "qqqq");
         test("no_more_tokens", !iter.FetchToken(word));

         ScriptVariable v2;
         v2 = "abc,  def, ghijh , q  ";
         ScriptVariable::Substring iter2(v2);
         test("first_token_at_begin", iter2.FetchToken(word));
         test_str("first_token_at_begin_value", word.Get().c_str(), "abc");

         ScriptVariable v3;
         v3 = "abc";
         ScriptVariable::Substring iter3(v3);
         test("first_token_the_only", iter3.FetchToken(word));
         test_str("first_token_the_only", word.Get().c_str(), "abc");
         test("first_token_the_only_ends", !iter3.FetchToken(word));

         ScriptVariable v4;
         v4 = " ";
         ScriptVariable::Substring iter4(v4);
         test("spaces_only_token_string", iter4.FetchToken(word, " "));
         test_str("empty_token", word.Get().c_str(), "");

         #if 0     // this is found to be inconsistent!
         ScriptVariable v5;
         ScriptVariable::Substring iter5(v5);
         test("empty_no_tokens", !iter5.FetchToken(word));
         #endif

         ScriptVariable v6(",,");
         ScriptVariable::Substring iter6(v6);
         test("empty_token_begin", iter6.FetchToken(word));
         test_str("empty_token_begin_empty", word.Get().c_str(), "");
         test("empty_token_middle", iter6.FetchToken(word));
         test_str("middle_token_middle_empty", word.Get().c_str(), "");
         test("empty_token_end", iter6.FetchToken(word));
         test_str("empty_token_end_empty", word.Get().c_str(), "");

         ScriptVariable v7("klkjds lsafj lfjd lfj");
         ScriptVariable::Substring iter7(v7);
         ScriptVariable::Substring iter7got =
                                    iter7.FetchToken("\n");
         test_str("whole_string", iter7got.Get().c_str(),
                                  v7.Whole().Get().c_str());

      }
      test_subsuite("iterator words");
      {
         ScriptVariable v1;
         v1 = " abc    def ghijh  q  ";
         ScriptVariable::Iterator iter(v1);
         test("first_word", iter.NextWord());
         test_str("first_word_value", iter.Get().c_str(), "abc");
         test("second_word", iter.NextWord());
         test_str("second_word_value", iter.Get().c_str(), "def");
         test("third_word", iter.NextWord());
         test_str("third_word_value", iter.Get().c_str(), "ghijh");
         test("fourth_word", iter.NextWord());
         test_str("fourth_word_value", iter.Get().c_str(), "q");
         test("no_more_words", !iter.NextWord());

         ScriptVariable v2;
         v2 = "abc    def ghijh  q  ";
         ScriptVariable::Iterator iter2(v2);
         test("first_word_at_begin", iter2.NextWord());
         test_str("first_word_at_begin_value", iter2.Get().c_str(), "abc");

         ScriptVariable v3;
         v3 = "abc";
         ScriptVariable::Iterator iter3(v3);
         test("first_word_the_only", iter3.NextWord());
         test_str("first_word_the_only", iter3.Get().c_str(), "abc");
         test("first_word_the_only_ends", !iter3.NextWord());

         ScriptVariable v4;
         v4 = "   ";
         ScriptVariable::Iterator iter4(v4);
         test("no_words", !iter4.NextWord());

         ScriptVariable v5;
         ScriptVariable::Iterator iter5(v5);
         test("empty_no_words", !iter5.NextWord());
      }
      test_subsuite("iterator tokens");
      {
         ScriptVariable v1;
         v1 = "abc,def,ghijh,q,qqqq";
         ScriptVariable::Iterator iter(v1);
         test("first_token", iter.NextToken());
         test_str("first_token_value", iter.Get().c_str(), "abc");
         test("second_token", iter.NextToken());
         test_str("second_token_value", iter.Get().c_str(), "def");
         test("third_token", iter.NextToken());
         test_str("third_token_value", iter.Get().c_str(), "ghijh");
         test("fourth_token", iter.NextToken());
         test_str("fourth_token_value", iter.Get().c_str(), "q");
         test("fifth_token", iter.NextToken());
         test_str("fifth_token_value", iter.Get().c_str(), "qqqq");
         test("no_more_tokens", !iter.NextToken());

         ScriptVariable v2;
         v2 = "abc,def,ghijh,q";
         ScriptVariable::Iterator iter2(v2);
         test("first_token_at_begin", iter2.NextToken());
         test_str("first_token_at_begin_value", iter2.Get().c_str(), "abc");

         ScriptVariable v3;
         v3 = "abc";
         ScriptVariable::Iterator iter3(v3);
         test("first_token_the_only", iter3.NextToken());
         test_str("first_token_the_only", iter3.Get().c_str(), "abc");
         test("first_token_the_only_ends", !iter3.NextToken());

         ScriptVariable v4;
         v4 = "   ";
         ScriptVariable::Iterator iter4(v4);
         test("spaces_only_token_string", iter4.NextToken(":", " "));
         test_str("empty_token", iter4.Get().c_str(), "");

         #if 0  // this found to be inconsistent!
         ScriptVariable v5;
         ScriptVariable::Iterator iter5(v5);
         test("empty_no_tokens", !iter5.NextToken());
         #endif

         ScriptVariable v6(",,");
         ScriptVariable::Iterator iter6(v6);
         test("empty_token_begin", iter6.NextToken());
         test_str("empty_token_begin_empty", iter6.Get().c_str(), "");
         test("empty_token_middle", iter6.NextToken());
         test_str("middle_token_middle_empty", iter6.Get().c_str(), "");
         test("empty_token_end", iter6.NextToken());
         test_str("empty_token_end_empty", iter6.Get().c_str(), "");
      }
      test_subsuite("iterator words backwards");
      {
         ScriptVariable v1;
         v1 = " abc    def ghijh  pqr  ";
         ScriptVariable::Iterator iter(v1);
         test("last_word", iter.PrevWord());
         test_str("last_word_value", iter.Get().c_str(), "pqr");
         test("second_word", iter.PrevWord());
         test_str("second_word_value", iter.Get().c_str(), "ghijh");
         test("third_word", iter.PrevWord());
         test_str("third_word_value", iter.Get().c_str(), "def");
         test("fourth_word", iter.PrevWord());
         test_str("fourth_word_value", iter.Get().c_str(), "abc");
         test("no_more_words", !iter.PrevWord());

         ScriptVariable v2;
         v2 = "  abc    def ghijh  pqr";
         ScriptVariable::Iterator iter2(v2);
         test("last_word_at_end", iter2.PrevWord());
         test_str("last_word_at_end_value", iter2.Get().c_str(), "pqr");

         ScriptVariable v3;
         v3 = "abc";
         ScriptVariable::Iterator iter3(v3);
         test("last_word_the_only", iter3.PrevWord());
         test_str("last_word_the_only", iter3.Get().c_str(), "abc");
         test("last_word_the_only_ends", !iter3.PrevWord());

         ScriptVariable v4;
         v4 = "   ";
         ScriptVariable::Iterator iter4(v4);
         test("no_words", !iter4.PrevWord());

         ScriptVariable v5;
         ScriptVariable::Iterator iter5(v5);
         test("empty_no_words", !iter5.PrevWord());
      }
      test_subsuite("iterator tokens backwards");
      {
         ScriptVariable v1;
         v1 = "abc,def,ghijh,q,pqr";
         ScriptVariable::Iterator iter(v1);
         test("last_token", iter.PrevToken());
         test_str("last_token_value", iter.Get().c_str(), "pqr");
         test("second_token", iter.PrevToken());
         test_str("second_token_value", iter.Get().c_str(), "q");
         test("third_token", iter.PrevToken());
         test_str("third_token_value", iter.Get().c_str(), "ghijh");
         test("fourth_token", iter.PrevToken());
         test_str("fourth_token_value", iter.Get().c_str(), "def");
         test("fifth_token", iter.PrevToken());
         test_str("fifth_token_value", iter.Get().c_str(), "abc");
         test("no_more_tokens", !iter.PrevToken());

         ScriptVariable v2;
         v2 = "abc,def,ghijh,pqr";
         ScriptVariable::Iterator iter2(v2);
         test("last_token_at_end", iter2.PrevToken());
         test_str("last_token_at_end_value", iter2.Get().c_str(), "pqr");

         ScriptVariable v3;
         v3 = "abc";
         ScriptVariable::Iterator iter3(v3);
         test("last_token_the_only", iter3.PrevToken());
         test_str("last_token_the_only", iter3.Get().c_str(), "abc");
         test("last_token_the_only_ends", !iter3.PrevToken());

         ScriptVariable v4;
         v4 = "   ";
         ScriptVariable::Iterator iter4(v4);
         test("spaces_only_token_string", iter4.PrevToken(":", " "));
         test_str("empty_token", iter4.Get().c_str(), "");

         #if 0  // this found to be inconsistent!
         ScriptVariable v5;
         ScriptVariable::Iterator iter5(v5);
         test("empty_no_tokens", !iter5.PrevToken());
         #endif

         ScriptVariable v6(",,");
         ScriptVariable::Iterator iter6(v6);
         test("back_empty_token_end", iter6.PrevToken());
         test_long("back_empty_token_len_zero", iter6.Length(), 0);
         test_str("back_empty_token_end_empty", iter6.Get().c_str(), "");
         test("back_empty_token_middle", iter6.PrevToken());
         test_str("middle_token_middle_back_empty", iter6.Get().c_str(), "");
         test("back_empty_token_begin", iter6.PrevToken());
         test_str("back_empty_token_begin_empty", iter6.Get().c_str(), "");
      }
      test_subsuite("iterator mixed words/tokens");
      {
         ScriptVariable v1;
         v1 = " justaword    ignoredword   :  secondtoken";
         ScriptVariable::Iterator iter(v1);
         test("mix_first_word", iter.NextWord());
         test_str("mix_first_token_value", iter.Get().c_str(), "justaword");
         test("mix_second_token", iter.NextToken(":", " "));
         test_str("mix_second_token_value", iter.Get().c_str(), "secondtoken");
      }
      test_subsuite("vsprintf");
      {
         ScriptVariable v1(10, "%d%d", 56, 78);
         test_str("vsprintf_sufficient_room", v1.c_str(), "5678");
         test_long("vsprintf_len", v1.length(), 4);
         ScriptVariable v2(10, "%d%s%d", 56,
         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
         78);
         test_str("vsprintf_insufficient_room", v2.c_str(), "56"
         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
         "78");
      }
      test_subsuite("numbers");
      {
          test_str("int_0", ScriptNumber(0).c_str(), "0");
          test_str("int_25", ScriptNumber(25).c_str(), "25");
          test_str("int_neg25", ScriptNumber(-25).c_str(), "-25");
          test_str("int_12345678", ScriptNumber(12345678).c_str(), "12345678");
          test_str("maxint", ScriptNumber(0x7fffffff).c_str(), "2147483647");
          int i = 0x80000001;
          //int i = -2147483647;
          test_str("minint_plus_1", ScriptNumber(i).c_str(), "-2147483647");
          i = 0x80000000;
          test_str("minint", ScriptNumber(i).c_str(), "-2147483648");
          unsigned int ui = 0x80000000;
          test_str("minint_unsigned", ScriptNumber(ui).c_str(), "2147483648");
          unsigned long long big = 0;
          big = ~big;
          big >>= 1;    // now it is max_long_long
          long long signed_big = big;
          test_str("maxlonglong", ScriptNumber(signed_big).c_str(),
                                  "9223372036854775807");
          signed_big = ~signed_big;   // now it is min_long_long
          test_str("minlonglong", ScriptNumber(signed_big).c_str(),
                                  "-9223372036854775808");
      }
      test_subsuite("getting_numbers");
      {
          {
          ScriptVariable v1("257");
          long x;
          v1.GetLong(x);
          test_long("get_long", x, 257);
          }
          {
          ScriptVariable v1("0x25bc");
          long x;
          v1.GetLong(x);
          test_long("get_hex_long", x, 0x25bc);
          }
          {
          ScriptVariable v1("17.15");
          long n,m;
          v1.GetRational(n, m);
          test_long("get_rational_n", n, 1715);
          test_long("get_rational_m", m, 100);
          }
      }
      test_subsuite("vector");
      {
          ScriptVector vec1(" word1     word2 word3   word4");
          test_str("word1", vec1[0].c_str(), "word1");
          test_str("word2", vec1[1].c_str(), "word2");
          test_str("word3", vec1[2].c_str(), "word3");
          test_str("word4", vec1[3].c_str(), "word4");
          test_str("join", vec1.Join().c_str(), "word1word2word3word4");
          test_str("join2", vec1.Join("->").c_str(),
                            "word1->word2->word3->word4");
          ScriptVector vec2(" token1  :   token2:: token3   :token4",
                            ":", " ");
          test_str("token1", vec2[0].c_str(), "token1");
          test_str("token2", vec2[1].c_str(), "token2");
          test_str("token3", vec2[2].c_str(), "");
          test_str("token4", vec2[3].c_str(), "token3");
          test_str("token5", vec2[4].c_str(), "token4");

          ScriptVector vec3("a b");
          vec3[15] = "c";
          test_long("index_changes_length", vec3.Length(), 16);

          ScriptVector vec4("      abrakadabra      ", ",", " \t\n\r");
          test_long("vector_from_single_token", vec4.Length(), 1);
          test_str("single_token_in_vector", vec4[0].c_str(), "abrakadabra");

          ScriptVector vec5;
          ScriptVariable abrakadabra("      abrakadabra      ");
          vec5 = ScriptVector(abrakadabra, ",", " \t\n\r");
          test_long("vector_from_single_token_assg", vec4.Length(), 1);
          test_str("single_token_in_vector_assg",
                                        vec4[0].c_str(), "abrakadabra");

          ScriptVector vec6("aaa bbb ccc ddd eee fff");
          test_str("vector_join_segment", vec6.Join(">>", 3).c_str(),
                                       "ddd>>eee>>fff");
          test_str("vector_join_segment2", vec6.Join(">>>", 3, 2).c_str(),
                                       "ddd>>>eee");
          test_str("vector_join_segment3", vec6.Join(">>>", 100, 200).c_str(),
                                       "");
          test_long("vector_join_segment3a", vec6.Length(), 6);
      }
      test_subsuite("subvector");
      {
          ScriptVector vec1("word1 word2 word3 word4");
          ScriptVector vec2(vec1, 2);
          test_str("second_half", vec2.Join(" ").c_str(), "word3 word4");
          ScriptVector vec3(vec1, 2, 1);
          test_str("part2_1", vec3.Join(" ").c_str(), "word3");
          ScriptVector vec4(vec1, 4, 1);
          test_str("outranged", vec4.Join(" ").c_str(), "");
          ScriptVector vec5(vec1, 2, 0);
          test_str("zerolength", vec5.Join(" ").c_str(), "");
      }
      test_subsuite("vector part remove");
      {
          {
          ScriptVector vec("word1 word2 word3 word4 word5");
          vec.Remove(1, 1);
          test_str("vector_remove_1_1", vec.Join(" ").c_str(),
                                                "word1 word3 word4 word5");
          }
          {
          ScriptVector vec("word1     word2    word3  word4     word5");
          vec.Remove(1, 3);
          test_str("vector_remove_1_3", vec.Join(" ").c_str(),
                                                "word1 word5");
          }
          {
          ScriptVector vec("word1     word2    word3  word4     word5");
          vec.Remove(2, 10);
          test_str("vector_remove_2_many", vec.Join(" ").c_str(),
                                                "word1 word2");
          }
          {
          ScriptVector vec("word1 word2 word3");
          vec.Remove(2, 1);
          test_str("vector_remove_last", vec.Join(" ").c_str(),
                                                "word1 word2");
          test_long("vector_remove_last_len", vec.Length(), 2);
          }
          {
          ScriptVector vec("word1 word2 word3");
          vec.Remove(2);
          test_str("vector_remove_last_all", vec.Join(" ").c_str(),
                                                "word1 word2");
          test_long("vector_remove_last_all_len", vec.Length(), 2);
          }

      }
      test_subsuite("vector larger remove");
      {
          int len;
          for(len = 20; len < 100; len++) {
              ScriptVector v;
              int i;
              for(i = 0; i < len; i++)
                  v.AddItem(ScriptVariable("str")+ScriptNumber(i));
              v.Remove(0, len-15);
              ScriptVariable testname =
                  ScriptVariable("vector_large_remove_") +
                  ScriptNumber(len);
              test_long(testname.c_str(), v.Length(), 15); 
          }
      }
      test_subsuite("vector insertion");
      {
          ScriptVector vec("word1 word2 word3 word4 word5 word6");
          int i;
          for(i = 0; i < 1000; i++)
               vec.Insert(3, ScriptVariable("inserted_")+ScriptNumber(i));
          test_long("vector_insertion_1000", vec.Length(), 1006);
      }
      test_subsuite("vector assignment");
      {
          ScriptVector vec("a b", " ");
          vec = ScriptVector("abra schwabra kadabra babra", " ,\t\n");
          test_long("vector_assignment_len", vec.Length(), 4);
          vec = ScriptVector("a", " ,\t\n");
          test_long("vector_assignment_len_shrink", vec.Length(), 1);
      }



      test_subsuite("resize bug");
      {
          {
          ScriptVariable v("/usr/src/linux");
          ScriptVariable::Substring s = v.Strrchr('/');
          s.Move(1);
          test_str("move_1", s.Get().c_str(), "l");
          test_long("move_idx", s.Index(), 9);
          test_long("move_len", s.Length(), 1);
          s.Resize(v.Length());
          test_str("resize_max", s.Get().c_str(), "linux");
          test_long("resize_max_idx", s.Index(), 9);
          //test_long("resize_max_len", s.Length(), 5);
          s.Erase();
          test_str("erase", v.c_str(), "/usr/src/");
          }
          {
          ScriptVariable v("0123456789");
          ScriptVariable::Substring s(v, 5, 15);
          s.Replace("abc");
          test_str("replace_long", v.c_str(), "01234abc");
          }
      }
      test_subsuite("extend");
      {
          ScriptVariable v("0123456789");
          ScriptVariable::Substring s(v, 5, 1);
          s.ExtendToBegin();
          test_str("extend_to_begin", s.Get().c_str(), "012345");
          ScriptVariable::Substring s2(v, 5, 1);
          s2.ExtendToEnd();
          test_str("extend_to_end", s2.Get().c_str(), "56789");
      }
      test_subsuite("setlength");
      {
          ScriptVariable v("0123456789");
          ScriptVariable::Substring s(v, 5, 2);
          s.SetLength(3);
          test_str("setlength", s.Get().c_str(), "567");
          ScriptVariable::Substring s2(v, 3, 3);
          s2.SetLength(500);
          test_str("setlength_too_many", s2.Get().c_str(), "3456789");
          ScriptVariable::Substring s3(v, 3, 3);
          s3.SetLength(-1);
          test_str("setlength_negative", s3.Get().c_str(), "3456789");
      }
      test_subsuite("numbers");
      {
          test_str("short", ScriptNumber((short int) 275).c_str(), "275");
          test_str("ushort", ScriptNumber((unsigned short) 57275).c_str(),
                                   "57275");
          test_str("int", ScriptNumber((int) 275).c_str(), "275");
          test_str("uint", ScriptNumber((unsigned int) 57275).c_str(),
                                   "57275");
          test_str("long", ScriptNumber((long) 275).c_str(), "275");
          test_str("ulong", ScriptNumber((unsigned long) 57275).c_str(),
                                   "57275");
          test_str("double", ScriptNumber(27.2727).c_str(), "27.2727");
          test_str("prec", ScriptNumber(27.123456789, 3).c_str(), "27.123");
          test_str("prec_l", ScriptNumber(27.123, 5).c_str(), "27.12300");

          ScriptNumber sn(751);
          ScriptVariable ns3 = sn+' '+sn+' '+sn;
          test_long("join_numbers_len", ns3.length(), 11);
          test_str("join_numbers", ns3.c_str(), "751 751 751");
      }
      test_subsuite("ScriptSet");
      {
          ScriptSet set;
          test_long("empty_set", set.Count(), 0);
          set.AddItem("Mary");
          set.AddItem("Jane");
          set.AddItem("Mr. Very Long Name, Jr.");
          test_long("set_count", set.Count(), 3);
          test("present", set.Contains("Jane"));
          test("present2", set.Contains("Mary"));
          test("not_present", !set.Contains("Harry"));
          test_long("set_count_unchanged", set.Count(), 3);
          int cw = 0;
          for(int i=0; i<1000; i++) {
              ScriptNumber sn(i);
              ScriptVariable str = sn + ' ' + sn + ' ' + sn;
              if(!set.AddItem(str))
                  printf("    COULDNT ADD %d\n", i);
              if(set.Count() != i + 4 - cw) {
                  printf("     WRONG: i==%d, count==%ld\n",
                         i, set.Count());
                  cw++;
              }
          }
          test_long("set_count_big", set.Count(), 1003);
          test("present_after_resize", set.Contains("Mary"));
          test("still_not_present", !set.Contains("Harry"));
          test("present_long", set.Contains("0 0 0"));
          test("present_long751", set.Contains("751 751 751"));
      }
      test_subsuite("ScriptMap");
      {
          ScriptMap map;
          test_long("empty_map", map.Count(), 0);
          map.AddItem("Mary", "Little Mary");
          map.AddItem("Jane", "Big Jane");
          map.AddItem("Mr. Very Long Name, Jr.", "Bobby");
          test_long("map_count", map.Count(), 3);
          test_str("get", map.GetItem("Jane").c_str(), "Big Jane");
          test_str("get2", map.GetItem("Mary").c_str(), "Little Mary");
          test("not_present", map.GetItem("Harry").IsInvalid());
          test_long("map_count_unchanged", map.Count(), 3);
          int cw = 0;
          for(int i=0; i<1000; i++) {
              ScriptNumber sn(i);
              ScriptVariable str = sn + ' ' + sn + ' ' + sn;
              if(!map.AddItem(str, sn))
                  printf("    COULDNT ADD %d\n", i);
              if(map.Count() != i + 4 - cw) {
                  printf("     WRONG: i==%d, count==%ld\n",
                         i, map.Count());
                  cw++;
              }
          }
          test_long("map_count_big", map.Count(), 1003);
          test_str("getafterresize", map.GetItem("Jane").c_str(),"Big Jane");
          test_str("getafterresize2", map.GetItem("Mary").c_str(),
                                                            "Little Mary");
          test("still_not_present", map.GetItem("Harry").IsInvalid());
          test_str("get_long", map.GetItem("0 0 0").c_str(), "0");
          test_str("get_long751", map.GetItem("751 751 751").c_str(), "751");


          ScriptMap m2;
          m2["Anna"] += "Little Ann";
          m2["Liza"] += "Pretty Liza";
          m2["Anna"] += " sleeps deep";
          test_str("index_operator", m2["Liza"].c_str(), "Pretty Liza");
          test_str("index_operator2", m2["Anna"].c_str(),
                                      "Little Ann sleeps deep");
      }




      test_subsuite("ScriptMacroprocessor");
      {
          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("foo", "well-bar"));
          ScriptVariable v;
          s.AddMacro(new ScriptMacroScrVar("x", &v));

          test_str("macro_const", s("foo%foo% %foo%").c_str(),
                                 "foowell-bar well-bar");
          test_str("macro_const_misname", s("foo%foor% %foo%").c_str(),
                                 "foo%foor% well-bar");
          test_str("macro_percent", s("foo%%foo %% %%foo%%").c_str(),
                                 "foo%foo % %foo%");

          ScriptVariableInv inval;
          test("macro_preserves_invalstring", s(inval).IsInvalid());

          v = "hell";
          test_str("macro_var", s("go to %x%!!").c_str(), "go to hell!!");
          v = "heaven";
          test_str("macro_var2", s("go to %x%!!").c_str(), "go to heaven!!");

          ScriptVector vv("shit happens twice");
          s.SetPositionals(vv, false);
          test_str("macro_argv", s("%2% %1% %0%").c_str(),
                                 "twice happens shit");
          test_str("macro_allargs", s("abc%*%def").c_str(),
                                 "abchappens twicedef");

          ScriptMacroprocessor s2;
          ScriptVector vv2(
              "shit happens twice and once again and then another time");
          s2.SetPositionals(vv2, 2, 4, false);
          test_str("macro_argv_idx", s2("%3% %2% %1% %0%").c_str(),
                                 "again once and twice");
          test_str("macro_allargs_idx", s2("%*%").c_str(),
                                 "and once again");

          ScriptMacroprocessor s3;
          ScriptVector vv3("shit happens twice");
          test_str("macro_with_argv", s3("%2% %1% %0%", vv3).c_str(),
                                 "twice happens shit");
          test_str("macro_with_argv_allargs", s3("abc%*%def", vv3).c_str(),
                                 "abchappens twicedef");
      }
      test_subsuite("ScriptMacroFunctions");
      {
          ScriptMacroprocessor s;
          s.AddMacro(new MacroFunc);
          test_str("macro_paramreq", s("foo%req:John:Doe:Idiot%bar").c_str(),
                                 "foo(John)(Doe)(Idiot)bar");
          test_str("macro_paramreq_noparam", s("foo%req%bar").c_str(),
                                 "foo<empty>bar");
      }
      test_subsuite("ScriptMacroNested");
      {
          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("f1", "foo"));
          s.AddMacro(new ScriptMacroConst("b1", "bar"));
          s.AddMacro(new ScriptMacroConst("foobar", "WellDone"));
          test_str("macro_no_nesting_in_names", s("ttt%[%f1%%b1%]eee").c_str(),
                                 "ttt%[%f1%%b1%]eee");

          ScriptMacroprocessor s2;
          s2.AddMacro(new MacroFunc);
          s2.AddMacro(new ScriptMacroConst("f1", "foo"));
          s2.AddMacro(new ScriptMacroConst("f2", "bar"));
          s2.AddMacro(new ScriptMacroConst("f3", "bur"));
          test_str("macro_nested2",
              s2("nnn%[req:%f1%:%[req:%f2%:%f3%]]aaa").c_str(),
                                 "nnn(foo)((bar)(bur))aaa");
      }
      test_subsuite("ScriptMacro only one level");
      {
          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("attack", "CRACKED"));
          s.AddMacro(new ScriptMacroConst("suspect", "%[attack]"));
          s.AddMacro(new ScriptMacroConst("unaware", "%[suspect]"));

          test_str("macro_args_in_eager_inj_safe",
                   s("ttt%[unaware]eee").c_str(),
                   "ttt%[suspect]eee");

      }
      test_subsuite("ScriptMacro dictionaries");
      {
          const char *v[] = { "foo", "bar", "abra", "kadabra", 0 };
          ScriptVector dd(v);
          ScriptMacroDictionary dict("dd", dd);

          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("a", "bcd"));

          test_str("dictionary",
              s.Process("%a% %dd:abra% %dd:foo%", &dict).c_str(),
              "bcd kadabra bar");
      }
      test_subsuite("ScriptMacro Lazy");
      {
          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("foobar", "foo:bar"));
          s.AddMacro(new ScriptMacroConst("abrakadabra", "abra:kadabra"));
          s.AddMacro(new MacroFunc);
          s.AddMacro(new MacroQuote);
          test_str("too_simple_lazy",
                   s("ttt%{foobar}eee").c_str(),
                   "tttfoo:bareee");
          test_str("simple_lazy",
                   s("ttt%{req:abc:def:ghi}eee").c_str(),
                   "ttt(abc)(def)(ghi)eee");
          test_str("simple_lazy_quoted_closer",
                   s("ttt%{req:abc:d%}f:ghi}eee").c_str(),
                   "ttt(abc)(d}f)(ghi)eee");
          test_str("macro_nested_eager_innder_delimiter_kept",
                   s("ttt%[req:%foobar%:%abrakadabra%]eee").c_str(),
                   "ttt(foo:bar)(abra:kadabra)eee");
          test_str("macro_nested_lazy",
                   s("ttt%{req:%foobar%:%abrakadabra% }eee").c_str(),
                   "ttt(foo:bar)(abra:kadabra )eee");
          test_str("macro_nested_lazy_esc_before_real_closer",
                   s("ttt%{req:%foobar%:%abrakadabra%}eee").c_str(),
                   "ttt(foo:bar)(abra:kadabra)eee");
          test_str("macro_nested_lazy_2",
                   s("ttt%{req:%foobar%:%[abrakadabra]}eee").c_str(),
                   "ttt(foo:bar)(abra:kadabra)eee");
          test_str("macro_nested_lazy_in_lazy",
                   s("ttt%{req: %{foobar} }eee").c_str(),
                   "ttt( foo:bar )eee");
          test_str("macro_nested_lazy_in_lazy_2",
                   s("ttt%{req:%{foobar}:%{abrakadabra}}eee").c_str(),
                   "ttt(foo:bar)(abra:kadabra)eee");
          test_str("macro_nested_lazy_in_lazy_with_esc",
                   s("ttt%{req:%{foobar}:%}:%{abrakadabra}}eee").c_str(),
                   "ttt(foo:bar)(})(abra:kadabra)eee");

          test_str("macro_lazy_in_eager", s("ttt%[req:%{foobar}]eee").c_str(),
                   "ttt(foo:bar)eee");

          test_str("macro_lazy_req_in_req", s("%{req:%{req:a}}").c_str(),
                   "((a))");

          test_str("quote", s("%[q:abc]").c_str(), "abc");

          test_str("macro_lazy_req_in_nr",
               s("x%[q:z%{req:a}t]y").c_str(), "xz(a)ty");

          ScriptVector vv("shit happens twice");
          test_str("macro_argv_in_lazy",
                   s("ttt%{req:%1%:%2%:%[0]}eee", vv).c_str(),
                   "ttt(happens)(twice)(shit)eee");
      }
      test_subsuite("ScriptMacro Lazy safety");
      {
          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("attack", "CRACKED"));
          s.AddMacro(new ScriptMacroConst("suspect", "%[attack]"));
          s.AddMacro(new MacroFunc);

          test_str("macro_injection_in_lazy",
                   s("ttt%{req:%{suspect}}eee").c_str(),
                   "ttt(CRACKED)eee");

          test_str("macro_eager_in_lazy_inj_safe",
                   s("ttt%{req:%[suspect]}eee").c_str(),
                   "ttt(%[attack])eee");

          test_str("macro_eager_in_double_lazy_inj_safe",
                   s("ttt%{req:a:%{req:%[suspect]}:b}eee").c_str(),
                   "ttt(a)((%[attack]))(b)eee");
      }
      test_subsuite("ScriptMacro eager argument injection protection");
      {
          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("attack", "CRACKED"));
          s.AddMacro(new ScriptMacroConst("suspect", "%[attack]"));
          s.AddMacro(new ScriptMacroConst("unaware", "%[suspect]"));
          s.AddMacro(new MacroFunc);

          test_str("macro_args_in_eager_inj_safe",
                   s("ttt%[req:%[unaware]:%[unaware]]eee").c_str(),
                   "ttt(%[suspect])(%[suspect])eee");

      }
      test_subsuite("ScriptMacro Lazy infinity protection");
      {
          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("a", "%{a}"));

          test_str("macro_loop_safety", s("%{a}").c_str(), "%{a}");
      }
      test_subsuite("ScriptMacro scoping");
      {
          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("abra", "kadabra"));
          ScriptMacroprocessor s2(&s);
          s2.AddMacro(new ScriptMacroConst("foo", "bar"));

          test_str("macro_scoped_processors", s2("%abra% %foo%").c_str(),
                                              "kadabra bar");
      }
      test_subsuite("ScriptMacro invalidated string");
      {
          fflush(stdout);
          ScriptMacroprocessor s;
          s.AddMacro(new MacroFunc);
          s.AddMacro(new ScriptMacroConst("inval", ScriptVariableInv()));

          test_str("macro_inval_tolerance", s("%[req:%inval%]").c_str(),
                                            "(%inval%)");
          test_str("macro_inval_tolerance2", s("%[req:a%inval%b]").c_str(),
                                            "(a%inval%b)");
      }
      test_subsuite("ScriptMacro unknown variable");
      {
          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("abra", "kadabra"));

          test_str("macro_keeps_unknown", s("%foo%").c_str(), "%foo%");
          test_str("macro_keeps_unkn2", s("a%foo%b").c_str(), "a%foo%b");
          test_str("macro_keeps_unkn3", s("%[foo]").c_str(), "%[foo]");
          test_str("macro_keeps_unkn4", s("a%[foo]b").c_str(), "a%[foo]b");
      }
      test_subsuite("ScriptMacro positionals after call");
      {
             // this test was added in suspection for a bug,
             // only to show there's no bug
          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("abra", "kadabra"));
          s.AddMacro(new MacroFunc);
          ScriptVector v("zero one two three", " ");
          ScriptVariable res = s.Process("a %0% %[req:%1%:b] c %2% d", v);
          test_str("macro_poss_after_call", res.c_str(),
                                         "a zero (one)(b) c two d");
      }
      test_subsuite("ScriptMacro Apply method");
      {
          ScriptMacroprocessor s;
          s.AddMacro(new ScriptMacroConst("abra", "kadabra"));
          s.AddMacro(new MacroFunc);
          s.AddMacro(new MacroFunc("bad", true));
          ScriptVector args;
          args.AddItem("foo");
          args.AddItem("bar");
          args.AddItem("bur");
          ScriptVariable res = s.Apply("req", args);
          test_str("macro_apply", res.c_str(), "(foo)(bar)(bur)");
          ScriptVariable res2 = s.Apply("bad", args);
          test("macro_apply_dirty_blocked", res2.IsInvalid());
      }
      test_subsuite("ScriptSubst");
      {
          ScriptSubstitution s;
          s.AddVar(new ScriptSubstitutionConst("foo", "well-bar"));
          ScriptVariable v;
          s.AddVar(new ScriptSubstitutionScrVar("x", &v));

          test_str("subs_const", s("foo%foo% %foo%").c_str(),
                                 "foowell-bar well-bar");
          test_str("subs_const_misname", s("foo%foo % %foo%").c_str(),
                                 "foo%foo % well-bar");
          test_str("subs_percent", s("foo%%foo %% %%foo%%").c_str(),
                                 "foo%foo % %foo%");

          v = "hell";
          test_str("subs_var", s("go to %x%!!").c_str(), "go to hell!!");
          v = "heaven";
          test_str("subs_var2", s("go to %x%!!").c_str(), "go to heaven!!");

          ScriptVector vv("shit happens twice");
          s.AddVar(new ScriptSubstitutionArgv(&vv, false));
          test_str("subs_argv", s("%2% %1% %0%").c_str(),
                                 "twice happens shit");
          test_str("subs_allargs", s("abc%*%def").c_str(),
                                 "abchappens twicedef");

          ScriptSubstitution s2;
          ScriptVector vv2(
              "shit happens twice and once again and then another time");
          s2.AddVar(new ScriptSubstitutionArgv(&vv2, false, 2, 4));
          test_str("subs_argv_idx", s2("%3% %2% %1% %0%").c_str(),
                                 "again once and twice");
          test_str("subs_allargs_idx", s2("%*%").c_str(),
                                 "and once again");

          ScriptSubstitution s3;
          ScriptVector vv3("shit happens twice");
          test_str("subs_with_argv", s3("%2% %1% %0%", vv3).c_str(),
                                 "twice happens shit");
          test_str("subs_with_argv_allargs", s3("abc%*%def", vv3).c_str(),
                                 "abchappens twicedef");
      }
      test_subsuite("ScriptSubstPrefixReq");
      {
          ScriptSubstitution s;
          s.AddVar(new PrefReq);
          test_str("subs_paramreq", s("foo%req:John:Doe:Idiot%bar").c_str(),
                                 "foo(John)(Doe)(Idiot)bar");
          test_str("subs_paramreq_require_delim", s("foo%req--%bar").c_str(),
                                 "foo%req--%bar");
      }
      test_subsuite("ScriptSubstNested");
      {
          ScriptSubstitution s;
          s.AddVar(new ScriptSubstitutionConst("f1", "foo"));
          s.AddVar(new ScriptSubstitutionConst("b1", "bar"));
          s.AddVar(new ScriptSubstitutionConst("foobar", "WellDone"));
          test_str("subs_nested", s("ttt%[%f1%%b1%]eee").c_str(),
                                 "tttWellDoneeee");

          ScriptSubstitution s2;
          s2.AddVar(new ScriptSubstitutionConst("f1", "foo"));
          s2.AddVar(new ScriptSubstitutionConst("foo15", "bar"));
          s2.AddVar(new ScriptSubstitutionConst("xfoobarz", "WellDone"));
          test_str("subs_nested2", s2("nnn%[xfoo%[%[f1]15]z]aaa").c_str(),
                                 "nnnWellDoneaaa");
      }
      test_subsuite("ScriptSubst Lazy");
      {
          ScriptSubstitution s;
          s.AddVar(new ScriptSubstitutionConst("foobar", "foo:bar"));
          s.AddVar(new ScriptSubstitutionConst("abrakadabra", "abra:kadabra"));
          s.AddVar(new PrefReq);
          test_str("subs_nested_eager",
                   s("ttt%[req:%foobar%:%abrakadabra%]eee").c_str(),
                   "ttt(foo)(bar)(abra)(kadabra)eee");
          test_str("subs_nested_lazy",
                   s("ttt%{req:%foobar%:%abrakadabra% }eee").c_str(),
                   "ttt(foo:bar)(abra:kadabra )eee");
          test_str("subs_nested_lazy_2",
                   s("ttt%{req:%foobar%:%[abrakadabra]}eee").c_str(),
                   "ttt(foo:bar)(abra:kadabra)eee");
          test_str("subs_nested_lazy_in_lazy",
                   s("ttt%{req:%{foobar}:%{abrakadabra}}eee").c_str(),
                   "ttt(foo:bar)(abra:kadabra)eee");
          test_str("subs_nested_lazy_in_lazy_with_esc",
                   s("ttt%{req:%{foobar}:%}:%{abrakadabra}}eee").c_str(),
                   "ttt(foo:bar)(})(abra:kadabra)eee");

          test_str("subs_lazy_in_eager", s("ttt%[req:%{foobar}]eee").c_str(),
                   "ttt(foo)(bar)eee");

          test_str("subs_lazy_req_in_req", s("%{req:%{req:a}}").c_str(),
                   "((a))");

          s.AddVar(new ScriptSubstitutionConst("(a)", "ok"));
          test_str("subs_lazy_req_in_nr", s("%[%{req:a}]").c_str(), "ok");

          ScriptVector vv("shit happens twice");
          test_str("subs_argv_in_lazy",
                   s("ttt%{req:%1%:%2%:%[0]}eee", vv).c_str(),
                   "ttt(happens)(twice)(shit)eee");
      }
      test_subsuite("ScriptSubst Lazy safety");
      {
          ScriptSubstitution s;
          s.AddVar(new ScriptSubstitutionConst("attack", "CRACKED"));
          s.AddVar(new ScriptSubstitutionConst("suspect", "%[attack]"));
          s.AddVar(new PrefReq);

          test_str("subs_injection_in_lazy",
                   s("ttt%{req:%{suspect}}eee").c_str(),
                   "ttt(CRACKED)eee");

          test_str("subs_eager_in_lazy_inj_safe",
                   s("ttt%{req:%[suspect]}eee").c_str(),
                   "ttt(%[attack])eee");

          test_str("subs_eager_in_double_lazy_inj_safe",
                   s("ttt%{req:a:%{req:%[suspect]}:b}eee").c_str(),
                   "ttt(a)((%[attack]))(b)eee");
      }
      test_subsuite("ScriptSubst Lazy infinity protection");
      {
          ScriptSubstitution s;
          s.AddVar(new ScriptSubstitutionConst("a", "%{a}"));

          fflush(stdout);
          test_str("subs_loop_safety", s("%{a}").c_str(), "%{a}");
      }
      test_subsuite("ScriptSubst dictionaries");
      {
          const char *v[] = { "foo", "bar", "abra", "kadabra", 0 };
          ScriptVector dd(v);
          ScriptSubstitutionDictionary dict(dd);

          ScriptSubstitution s;
          s.AddVar(new ScriptSubstitutionConst("a", "bcd"));

          test_str("dictionary",
              s.Substitute("%a% %abra% %foo%", &dict).c_str(),
              "bcd kadabra bar");
      }
      test_subsuite("ScriptSubst keep unreplaced");
      {
          ScriptSubstitution s;
          s.AddVar(new ScriptSubstitutionConst("a", "XYZ"));

          test_str("simple_kept", s.Substitute("%a% %bcde%").c_str(),
              "XYZ %bcde%");
          test_str("lazy_inmost_kept", s.Substitute("%{a} %{bcde}").c_str(),
              "XYZ %{bcde}");
          test_str("active_inmost_kept", s.Substitute("%[a] %[bcde]").c_str(),
              "XYZ %[bcde]");
      }
      test_subsuite("ReadConfigFile");
      {
          const char *fnm = "___test";
          FILE *f = fopen(fnm, "w");
          if(f) {
              fputs("foo = bar\n"
                    "abra = schwabra\n+   kadabra\n"
                    "punk = is\n not\n dead\n", f);
              fclose(f);

              ReadConfigFileToVector rcf;
              rcf.FOpen(fnm);
              ScriptVector v;
              bool res = rcf.ReadTo(v);
              test("conffile_to_vec_success", res);
              test_long("conffile_to_vec_count", v.Length(), 6);
              test_str("conffile_to_vec_name1", v[0].c_str(), "foo");
              test_str("conffile_to_vec_name2", v[2].c_str(), "abra");
              test_str("conffile_to_vec_name3", v[4].c_str(), "punk");
              test_str("conffile_to_vec_simple_val", v[1].c_str(), "bar");
              test_str("conffile_to_vec_plus_val", v[3].c_str(),
                                                   "schwabra\n   kadabra");
              test_str("conffile_to_vec_spc_val", v[5].c_str(),
                                                   "is\nnot\ndead");

              f = fopen(fnm, "w");
              fputs("name15 = value15\n", f);
              fclose(f);
              rcf.FOpen(fnm);
              res = rcf.ReadTo(v);
              test("conffile_to_vec_again_success", res);
              test_str("conffile_to_vec_again_name", v[6].c_str(), "name15");
              test_str("conffile_to_vec_again_val", v[7].c_str(), "value15");

              unlink(fnm);
          } else {
              fprintf(stderr, "couldn't create test file!\n");
          }
      }
      test_subsuite("ReadConfigFile2");
      {
          const char *fnm = "___test";
          FILE *f = fopen(fnm, "w");
          if(f) {
              fputs("alpha = sunday\n"
                    "beta = monday\n"
                    "gamma = tuesday\n"
                    "foo = bar %{alpha} bur\n", f);
              fclose(f);
              ConfigInformation info;
              info.FOpen(fnm);
              info.RunParser();
              test_str("confinfo_val", info.GetValue("foo").c_str(),
                                       "bar sunday bur");

              unlink(fnm);
          } else {
              fprintf(stderr, "couldn't create test file!\n");
          }
      }
      test_subsuite("HeadedTextMessage");
      {
#define BODYBODY "Now the body comes into play\n"
          HeadedTextMessage msg(false);
          const char msgtext[] =
              "JustAHeader: just a value\n"
              "AnotherLongerHeader: it has its value, too\n"
              "ThisIsFoldedHeader: the value of this header\n"
              "    is so long that it has to be folded\n"
              "\n"
              BODYBODY;
          const char *str = msgtext;
          while(*str) {
              msg.FeedChar(*str);
              str++;
          }
          msg.FeedChar(-1);
          int errcode, errline;
          test("parsing_ok", !msg.Error(errcode, errline));
          test_long("has_three_headers", msg.GetHeaders().Length(), 6);
          test_long("body_length", msg.GetBody().Length(), sizeof(BODYBODY)-1);
          test_str("first_header", msg.GetHeaders()[0].c_str(), "JustAHeader");
          test_str("first_header_val", msg.GetHeaders()[1].c_str(),
                                                              "just a value");
          test_str("last_header", msg.GetHeaders()[4].c_str(),
                                                       "ThisIsFoldedHeader");
          test_str("folded_value", msg.GetHeaders()[5].c_str(),
             "the value of this header\nis so long that it has to be folded");
#undef BODYBODY
      }
      test_subsuite("HeadedTextMessage2");
      {
          HeadedTextMessage msg(false);
          const char msgtext[] =
              "JustAHeader: just a value\n"
              "AnotherLongerHeader: it has its value, too\n";
          const char *str = msgtext;
          while(*str) {
              msg.FeedChar(*str);
              if(msg.InBody()) {
                  test("not_in_body", false);
                  break;
              }
              str++;
          }
          test("not_in_body", true);
          msg.FeedChar('\n');
          test("in_body", msg.InBody());
          const char bodytext[] = "this\n is \n a body\n";
          str = bodytext;
          while(*str) {
              msg.FeedChar(*str);
              if(!msg.InBody()) {
                  test("in_body2", false);
                  break;
              }
              str++;
          }
          test("in_body2", true);
      }
      test_subsuite("HeadedTextMessage3");
      {
          HeadedTextMessage msg(false);
          const char msgtext[] =
              "JustAHeader: just a value\n"
              "AnotherLongerHeader: it has its value, too\n"
              "ThisIsFoldedHeader: the value of this header\n"
              "    is so long that it has to be folded\n"
              "This is broken header\n"
              "\n"
              "And here goes the body\n";
          const char *str = msgtext;
          while(*str) {
              msg.FeedChar(*str);
              str++;
          }
          msg.FeedChar(-1);
          int errcode, errline;
          test("parsing_fails", msg.Error(errcode, errline));
      }
      test_subsuite("HeadedTextMessage4");
      {
          HeadedTextMessage msg(false);
          const char msgtext[] =
              "JustAHeader: just a value\n"
              "AnotherLongerHeader: it has its value, too\n"
              "WellDouble: alpha\n"
              "ThisIsFoldedHeader: the value of this header\n"
              "    is so long that it has to be folded\n"
              "WellDouble: beta\n"
              "\n"
              "And here goes the body\n";
          const char *str = msgtext;
          while(*str) {
              msg.FeedChar(*str);
              str++;
          }
          msg.FeedChar(-1);
          test_str("get_header", msg.FindHeader("WellDouble").c_str(), "alpha");
          ScriptVector h;
          msg.FindHeaders("WellDouble", h);
          test_long("get_multiheader_num", h.Length(), 2);
          test_str("get_multiheader_1", h[0].c_str(), "alpha");
          test_str("get_multiheader_2", h[1].c_str(), "beta");
      }
      test_subsuite("HeadedTextMessage5");
      {
          HeadedTextMessage msg(true);
          const char msgtext[] =
              "from me to you\n"
              "JustAHeader: just a value\n"
              "AnotherLongerHeader: it has its value, too\n"
              "WellDouble: alpha\n"
              "Abra: kadabra\n"
              "ThisIsFoldedHeader: the value of this header\n"
              "    is so long that it has to be folded\n"
              "WellDouble: beta\n"
              "\n"
              "And here goes the body\n";
          const char *str = msgtext;
          while(*str) {
              msg.FeedChar(*str);
              str++;
          }
          msg.FeedChar(-1);
          test_long("removing_headers", msg.RemoveHeader("WellDouble"), 2);
          test_long("removing_headers1", msg.RemoveHeader("Abra"), 1);
          test_long("removing_headers0", msg.RemoveHeader("Abrakadabra"), 0);
          msg.SetHeader("JustAHeader", "another value");
          msg.SetHeader("AddedHeader", "must go last\neven in two lines");
          msg.SetHeadingLine("from you to me");
          msg.SetBody("Yeah, the body here goes\n");
          ScriptVariable ser = msg.Serialize();
          const char *expect =
              "from you to me\n"
              "JustAHeader: another value\n"
              "AnotherLongerHeader: it has its value, too\n"
              "ThisIsFoldedHeader: the value of this header\n"
              "\tis so long that it has to be folded\n"
              "AddedHeader: must go last\n"
              "\teven in two lines\n"
              "\n"
              "Yeah, the body here goes\n";
          const char *p = ser.c_str();
          int i;
          for(i = 0; ; i++) {
              if(p[i] != expect[i]) {
                  printf("DIFFERENCE at %d\n", i);
                  printf("E:[%s]\nR:[%s]\n", expect+i, p+i);
                  break;
              }
              if(!p[i])
                  break;
          }
          test_str("serialization_after_manipulations", p, expect);
      }
      test_subsuite("HeadedTextMessage6");
      {
          HeadedTextMessage msg(false);
          const char msgtext[] =
              "One: \xC8\xD5\xCA\xFF\n"
              "Two: \xD0\xC9\xDA\xFF\xC4\xC1\n"
              "\n"
              "body\n";
          const char *str = msgtext;
          while(*str) {
              msg.FeedChar((unsigned char)*str);
              str++;
          }
          test_str("header_with_xFF", msg.FindHeader("One").c_str(),
                                      "\xC8\xD5\xCA\xFF");
          test_str("header_with_xFF_in_middle", msg.FindHeader("Two").c_str(),
                                      "\xD0\xC9\xDA\xFF\xC4\xC1");
          test_str("body_after_xFF", msg.GetBody().c_str(), "body\n");
      }
      test_score();
  }
  catch(...) {
      printf("Something strange caught\n");
  }
  return 0;
}


