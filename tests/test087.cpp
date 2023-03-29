// test087.cpp - numeric c locale

#include <clocale>
#include <rapidcsv/rapidcsv.h>
#include "unittest.h"

int main()
{
  int rv = 0;

  std::string loc = "de_DE.UTF-8"; // uses comma (,) as decimal separator
  if (std::setlocale(LC_ALL, loc.c_str()) == nullptr)
  {
    std::cout << "locale " << loc << " not available, skipping test.\n";
    // pass test for systems without locale present. for ci testing, make.sh
    // ensures that the necessary locale is installed.
    return 0;
  }

  std::string path = unittest::TempPath();

  try
  {
    {
      std::string csv =
        "-;A;B;C\n"
        "1;1;10;100\n"
        "2;0,1;0,01;0,001\n"
      ;

      unittest::WriteFile(path, csv);

      rapidcsv::Document doc(path, rapidcsv::LabelParams(0, 0),
                             rapidcsv::SeparatorParams(';' /* pSeparator */));
      unittest::ExpectEqual(float, doc.GetCell<float>("A", "2"), 0.1f);
      unittest::ExpectEqual(float, doc.GetCell<float>("B", "2"), 0.01f);
      unittest::ExpectEqual(float, doc.GetCell<float>("C", "2"), 0.001f);
    }

    {
      std::string csv =
        "-,A,B,C\n"
        "1,1,10,100\n"
        "2,0.1,0.01,0.001\n"
      ;

      unittest::WriteFile(path, csv);

      rapidcsv::LabelParams labelParams(0, 0);
      rapidcsv::SeparatorParams separatorParams;
      rapidcsv::Document doc(path, labelParams, separatorParams);
      // Note: 'ExceptEqual' is a macro-function; comma in "<float, 0, 0>" messes up the call.
      // Using macro-variable 'COMMA' is a workaround "<float COMMA 0 COMMA 0>".
      // doc.GetCell<float COMMA 0 COMMA 0>  ->  doc.GetCell<float, USE_NUMERIC_LOCALE=0, 0> : do not honor numeric locale
      unittest::ExpectEqual(float, doc.GetCell<float COMMA 0 COMMA 0>("A", "2"), 0.1f);
      unittest::ExpectEqual(float, doc.GetCell<float COMMA 0 COMMA 0>("B", "2"), 0.01f);
      unittest::ExpectEqual(float, doc.GetCell<float COMMA 0 COMMA 0>("C", "2"), 0.001f);
    }
  }
  catch (const std::exception& ex)
  {
    std::cout << ex.what() << std::endl;
    rv = 1;
  }

  unittest::DeleteFile(path);

  return rv;
}
