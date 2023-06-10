/*
 * rapidcsv.h
 *
 * URL:      https://github.com/panchaBhuta/rapidcsv_FilterSort
 * Version:  v2.01.fs-8.75
 *
 * Copyright (C) 2022-2023 Gautam Dhar
 * All rights reserved.
 * rapidcsv_FilterSort is distributed under the BSD 3-Clause license, see LICENSE for details.
 *
 *
 * ***********************************************************************************
 *
 * URL:      https://github.com/d99kris/rapidcsv
 * Version:  8.75
 *
 * Copyright (C) 2017-2023 Kristofer Berggren
 * All rights reserved.
 *
 * rapidcsv is distributed under the BSD 3-Clause license, see LICENSE for details.
 *
 */

#pragma once

#include <algorithm>

#ifdef HAS_CODECVT
#include <codecvt>
#include <locale>
#endif

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <tuple>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <rapidcsv/converter.h>

namespace rapidcsv
{
#if defined(_MSC_VER)
  static const bool sPlatformHasCR = true;
#else
  static const bool sPlatformHasCR = false;
#endif
  static const std::vector<char> s_Utf8BOM = { '\xef', '\xbb', '\xbf' };


  /**
   * @brief     Datastructure holding parameters controlling which row and column should be
   *            treated as labels.
   */
  struct LabelParams
  {
    /**
     * @brief   Constructor
     * @param   pColumnNameIdx        specifies the zero-based row index of the column labels, setting
     *                                it to -1 prevents column lookup by label name, and gives access
     *                                to all rows as document data. Default: 0
     * @param   pRowNameIdx           specifies the zero-based column index of the row labels, setting
     *                                it to -1 prevents row lookup by label name, and gives access
     *                                to all columns as document data. Default: -1
     */
    explicit LabelParams(const ssize_t pColumnNameIdx = 0, const ssize_t pRowNameIdx = -1)
      : mColumnNameIdx(pColumnNameIdx)
      , mRowNameIdx(pRowNameIdx)
    {
      if (mColumnNameIdx < -1)
      {
        std::string errStr = std::string(__RAPIDCSV_FILE__)+":"+std::to_string(__LINE__)+
          " invalid column name index " + std::to_string(mColumnNameIdx) + " < -1";
        throw std::out_of_range(errStr);
      }

      if (mRowNameIdx < -1)
      {
        const std::string errStr = std::string(__RAPIDCSV_FILE__)+":"+std::to_string(__LINE__)+
          " invalid row name index " + std::to_string(mRowNameIdx) + " < -1";
        throw std::out_of_range(errStr);
      }
    }

    /**
     * @brief   specifies the zero-based row index of the column labels.
     */
    ssize_t mColumnNameIdx;

    /**
     * @brief   specifies the zero-based column index of the row labels.
     */
    ssize_t mRowNameIdx;
  };

  /**
   * @brief     Datastructure holding parameters controlling how the CSV data fields are separated.
   */
  struct SeparatorParams
  {
    /**
     * @brief   Constructor
     * @param   pSeparator            specifies the column separator (default ',').
     * @param   pTrim                 specifies whether to trim leading and trailing spaces from
     *                                cells read (default false).
     * @param   pHasCR                specifies whether a new document (i.e. not an existing document read)
     *                                should use CR/LF instead of only LF (default is to use standard
     *                                behavior of underlying platforms - CR/LF for Win, and LF for others).
     * @param   pQuotedLinebreaks     specifies whether to allow line breaks in quoted text (default false)
     * @param   pAutoQuote            specifies whether to automatically dequote data during read, and add
     *                                quotes during write (default true).
     * @param   pQuoteChar            specifies the quote character (default '\"').
     */
    explicit SeparatorParams(const char pSeparator = ',', const bool pTrim = false,
                             const bool pHasCR = sPlatformHasCR, const bool pQuotedLinebreaks = false,
                             const bool pAutoQuote = true, const char pQuoteChar = '"')
      : mSeparator(pSeparator)
      , mTrim(pTrim)
      , mHasCR(pHasCR)
      , mQuotedLinebreaks(pQuotedLinebreaks)
      , mAutoQuote(pAutoQuote)
      , mQuoteChar(pQuoteChar)
    {}

    /**
     * @brief   specifies the column separator.
     */
    char mSeparator;

    /**
     * @brief   specifies whether to trim leading and trailing spaces from cells read.
     */
    bool mTrim;

    /**
     * @brief   specifies whether new documents should use CR/LF instead of LF.
     */
    bool mHasCR;

    /**
     * @brief   specifies whether to allow line breaks in quoted text.
     */
    bool mQuotedLinebreaks;

    /**
     * @brief   specifies whether to automatically dequote cell data.
     */
    bool mAutoQuote;

    /**
     * @brief   specifies the quote character.
     */
    char mQuoteChar;
  };

  /**
   * @brief     Datastructure holding parameters controlling how special line formats should be
   *            treated.
   */
  struct LineReaderParams
  {
    /**
     * @brief   Constructor
     * @param   pSkipCommentLines     specifies whether to skip lines prefixed with
     *                                mCommentPrefix. Default: false
     * @param   pCommentPrefix        specifies which prefix character to indicate a comment
     *                                line. Default: #
     * @param   pSkipEmptyLines       specifies whether to skip empty lines. Default: false
     */
    explicit LineReaderParams(const bool pSkipCommentLines = false,
                              const char pCommentPrefix = '#',
                              const bool pSkipEmptyLines = false)
      : mSkipCommentLines(pSkipCommentLines)
      , mCommentPrefix(pCommentPrefix)
      , mSkipEmptyLines(pSkipEmptyLines)
    {}

    /**
     * @brief   specifies whether to skip lines prefixed with mCommentPrefix.
     */
    bool mSkipCommentLines;

    /**
     * @brief   specifies which prefix character to indicate a comment line.
     */
    char mCommentPrefix;

    /**
     * @brief   specifies whether to skip empty lines.
     */
    bool mSkipEmptyLines;
  };

  struct RawIdx
  {
    const size_t dataIdx;
  };

  template <typename SI>
  concept c_sizet_or_string = std::is_convertible_v<SI,size_t> ||
                              std::is_convertible_v<SI,std::string>;


  /**
   * @brief     Class representing a CSV document.
   */
  class Document
  {
  public:

    using t_dataRow = std::vector<std::string>;
    typedef bool (* f_EvalBoolExpr)(const t_dataRow& dataRow);

    /**
     * @brief   Constructor
     * @param   pPath                 specifies the path of an existing CSV-file to populate the Document
     *                                data with.
     * @param   pLabelParams          specifies which row and column should be treated as labels.
     * @param   pSeparatorParams      specifies which field and row separators should be used.
     * @param   pLineReaderParams     specifies how special line formats should be treated.
     */
    explicit Document(const std::string& pPath = std::string(),
                      const LabelParams& pLabelParams = LabelParams(),
                      const SeparatorParams& pSeparatorParams = SeparatorParams(),
                      const LineReaderParams& pLineReaderParams = LineReaderParams())
      : _mPath(pPath)
      , _mLabelParams(pLabelParams)
      , _mSeparatorParams(pSeparatorParams)
      , _mLineReaderParams(pLineReaderParams)
      , _mData()
      , _mColumnNames()
      , _mRowNames()
    {
      if (!_mPath.empty())
      {
        _readCsv();
      }
    }

    /**
     * @brief   Constructor
     * @param   pStream               specifies a binary input stream to read CSV data from.
     * @param   pLabelParams          specifies which row and column should be treated as labels.
     * @param   pSeparatorParams      specifies which field and row separators should be used.
     * @param   pLineReaderParams     specifies how special line formats should be treated.
     */
    explicit Document(std::istream& pStream,
                      const LabelParams& pLabelParams = LabelParams(),
                      const SeparatorParams& pSeparatorParams = SeparatorParams(),
                      const LineReaderParams& pLineReaderParams = LineReaderParams())
      : _mPath()
      , _mLabelParams(pLabelParams)
      , _mSeparatorParams(pSeparatorParams)
      , _mLineReaderParams(pLineReaderParams)
      , _mData()
      , _mColumnNames()
      , _mRowNames()
    {
      _readCsv(pStream);
    }

    /**
     * @brief   Read Document data from file.
     * @param   pPath                 specifies the path of an existing CSV-file to populate the Document
     *                                data with.
     * @param   pLabelParams          specifies which row and column should be treated as labels.
     * @param   pSeparatorParams      specifies which field and row separators should be used.
     * @param   pLineReaderParams     specifies how special line formats should be treated.
     */
    void Load(const std::string& pPath,
              const LabelParams& pLabelParams = LabelParams(),
              const SeparatorParams& pSeparatorParams = SeparatorParams(),
              const LineReaderParams& pLineReaderParams = LineReaderParams())
    {
      _mPath = pPath;
      _mLabelParams = pLabelParams;
      _mSeparatorParams = pSeparatorParams;
      _mLineReaderParams = pLineReaderParams;
      _readCsv();
    }

    /**
     * @brief   Read Document data from stream.
     * @param   pStream               specifies a binary input stream to read CSV data from.
     * @param   pLabelParams          specifies which row and column should be treated as labels.
     * @param   pSeparatorParams      specifies which field and row separators should be used.
     * @param   pLineReaderParams     specifies how special line formats should be treated.
     */
    void Load(std::istream& pStream,
              const LabelParams& pLabelParams = LabelParams(),
              const SeparatorParams& pSeparatorParams = SeparatorParams(),
              const LineReaderParams& pLineReaderParams = LineReaderParams())
    {
      _mPath = "";
      _mLabelParams = pLabelParams;
      _mSeparatorParams = pSeparatorParams;
      _mLineReaderParams = pLineReaderParams;
      _readCsv(pStream);
    }

    /**
     * @brief   Write Document data to file.
     * @param   pPath                 optionally specifies the path where the CSV-file will be created
     *                                (if not specified, the original path provided when creating or
     *                                loading the Document data will be used).
     */
    void Save(const std::string& pPath = std::string())
    {
      if (!pPath.empty())
      {
        _mPath = pPath;
      }
      _writeCsv();
    }

    /**
     * @brief   Write Document data to stream.
     * @param   pStream               specifies a binary output stream to write the data to.
     */
    void Save(std::ostream& pStream) const
    {
      _writeCsv(pStream);
    }

    /**
     * @brief   Clears loaded Document data.
     *
     */
    void Clear()
    {
      _mData.clear();
      _mColumnNames.clear();
      _mRowNames.clear();
#ifdef HAS_CODECVT
      mIsUtf16 = false;
      mIsLE = false;
#endif
      _mHasUtf8BOM = false;
    }

    constexpr size_t GetColumnIdx(const size_t pColumnIdx) const
    {
      return pColumnIdx;
    }
    constexpr size_t GetColumnIdx(int pColumnIdx) const
    {
      return static_cast<size_t>(pColumnIdx);
    }
    /**
     * @brief   Get column index by name.
     * @param   pColumnName           column label name.
     * @returns zero-based column index.
     */
    size_t GetColumnIdx(const std::string& pColumnName) const
    {
      ssize_t columnIdx = -1;
      if (_mLabelParams.mColumnNameIdx >= 0)
      {
        if (_mColumnNames.find(pColumnName) != _mColumnNames.end())
        {
          columnIdx = static_cast<ssize_t>(_mColumnNames.at(pColumnName)) - static_cast<ssize_t>(_getDataColumnIndex(0).dataIdx);
        }
      }
      if (columnIdx < 0)
      {
        throw std::out_of_range(std::string(__RAPIDCSV_FILE__)+":"+std::to_string(__LINE__)+" column not found: " + pColumnName);
      }
      return static_cast<size_t>(columnIdx);
    }

    /*
    https://en.cppreference.com/w/cpp/language/template_parameters
    template<typename T, typename ...ARGS,
             T (*CONV_S2T)(const std::string&, ARGS ...) =
                       &ConvertFromStr<T, S2T_DefaultFormat<T>, ARGS...>::ToVal >
    std::vector<T> GetColumn(const auto& pColumnNameIdx,
                             ARGS&& ... args) const
    */
    // https://en.cppreference.com/w/cpp/language/template_parameters
    /**
     * @brief   Get column either by it's index or name.
     * @tparam  T                     'type' to converted to, using string data.
     * @tparam  S2Tconv               conversion class statisfying concept 'c_S2Tconverter'.
     *                                optional default argument = ConvertFromStr<T>
     *                                other possibilities are ConvertFromStr_fNaN<T> or ConvertFromStr_gNaN<T>
     * @param   pColumnNameIdx        column-name or zero-based column-index.
     * @returns vector<R> of column data. By default, R is same type as T.
     *          Else if S2Tconv = ConvertFromStr_gNaN<T>, then R = std::variant<T, std::string>
     *          On conversion success variant has the converted value, 
     *          else the string value which caused failure during conversion.
     */
    // TODO function and unit tests  for ARGS...
    template< c_S2Tconverter S2Tconv >
    std::vector<typename S2Tconv::return_type>
    GetColumn(const c_sizet_or_string auto& pColumnNameIdx) const
    {
      const size_t pColumnIdx = GetColumnIdx(pColumnNameIdx);
      const size_t dataColumnIdx = _getDataColumnIndex(pColumnIdx).dataIdx;
      std::vector<typename S2Tconv::return_type> column;
      size_t rowIdx = _getDataRowIndex(0).dataIdx;
      for (auto itRow = _mData.begin() + static_cast<ssize_t>(_getDataRowIndex(0).dataIdx);
           itRow != _mData.end(); ++itRow, ++rowIdx)
      {
        if (dataColumnIdx < itRow->size())
        {
          const std::string& cellStrVal = itRow->at(dataColumnIdx);
          //typename S2Tconv::return_type val = S2Tconv::ToVal_args(cellStrVal, std::forward<ARGS>(args)...);
          typename S2Tconv::return_type val = S2Tconv::ToVal(cellStrVal);
          column.push_back(val);
        } else {
          const std::string errStr = std::string(__RAPIDCSV_FILE__)+":"+std::to_string(__LINE__)+
            " Document::GetColumn() # requested column index " +
            std::to_string(pColumnIdx) + " >= " +
            std::to_string(itRow->size() - _getDataColumnIndex(0).dataIdx) +
            " (number of columns on row index " + std::to_string(rowIdx-_getDataColumnIndex(0).dataIdx) + ")";
          throw std::out_of_range(errStr);
        }
      }
      return column;
    }

    template< c_NOT_S2Tconverter T >
    inline std::vector< typename ConvertFromStr<T>::return_type >
    GetColumn(const c_sizet_or_string auto& pColumnNameIdx) const
    {
      return GetColumn<ConvertFromStr<T>>(pColumnNameIdx);
    }

    /**
     * @brief   Get column either by it's index or name.
     * @tparam  T                     'type' to converted to, using string data.
     * @tparam  CONV_S2T              conversion function.
     * @param   pColumnNameIdx        column-name or zero-based column-index.
     * @returns vector<R> of column data. By default, R is same type as T.
     *          Else if CONV_S2T is similar to function ConvertFromStr_gNaN<T>::ToVal, then R = std::variant<T, std::string>
     *          On conversion success variant has the converted value, 
     *          else the string value which caused failure during conversion.
     */
    template< typename T, auto (*CONV_S2T)(const std::string&) >
    inline std::vector< typename std::invoke_result_t< decltype(CONV_S2T),
                                                       const std::string&
                                                     >
                      >
    GetColumn(const c_sizet_or_string auto& pColumnNameIdx) const
    {
      return GetColumn< S2TwrapperFunction<T, CONV_S2T> >(pColumnNameIdx);
    }

    /**
     * @brief   Set column either by it's index or name.
     * @tparam  T                     'type' to converted to, using string data.
     * @tparam  S2Tconv               conversion class statisfying concept 'c_T2Sconverter'.
     *                                optional default argument = ConvertFromVal<T>
     *                                other possibilities is ConvertFromVal_gNaN<T>
     * @param   pColumnNameIdx        column-name or zero-based column-index.
     * @param   pColumn               vector of column data.
     */
    template< c_T2Sconverter T2Sconv >
    void SetColumn(const c_sizet_or_string auto& pColumnNameIdx,
                   const std::vector<typename T2Sconv::input_type>& pColumn)
    {
      const size_t pColumnIdx = GetColumnIdx(pColumnNameIdx);
      const size_t dataColumnIdx = _getDataColumnIndex(pColumnIdx).dataIdx;

      while (_getDataRowIndex(pColumn.size()).dataIdx > _getDataRowCount())
      {
        t_dataRow row;
        row.resize(_getDataColumnCount());
        _mData.push_back(row);
      }

      if ((dataColumnIdx + 1) > _getDataColumnCount())
      {
        for (auto itRow = _mData.begin(); itRow != _mData.end(); ++itRow) // + static_cast<ssize_t>(_getDataRowIndex(0).dataIdx)
        {
          itRow->resize(_getDataColumnIndex(dataColumnIdx + 1).dataIdx);
        }
      }

      size_t rowIdx = _getDataRowIndex(0).dataIdx;
      for (auto itCell = pColumn.begin(); itCell != pColumn.end(); ++itCell, ++rowIdx)
      {
        _mData.at(rowIdx).at(dataColumnIdx) = T2Sconv::ToStr(*itCell);
      }
    }

    template< c_NOT_T2Sconverter T >
    inline void
    SetColumn(const c_sizet_or_string auto& pColumnNameIdx,
                   const std::vector<typename ConvertFromVal<T>::input_type>& pColumn)
    {
      SetColumn<ConvertFromVal<T>>(pColumnNameIdx, pColumn);;
    }

    /**
     * @brief   Set column either by it's index or name.
     * @tparam  T                     'type' to converted to, using string data.
     * @tparam  CONV_T2S              conversion function.
     * @param   pColumnNameIdx        column-name or zero-based column-index.
     * @param   pColumn               vector of column data.
     */
    template< typename T, typename R, std::string (*CONV_T2S)(const R&) >
    inline void SetColumn(const c_sizet_or_string auto& pColumnNameIdx,
                          const std::vector<R>& pColumn)
    {
      using T2Swrap = T2SwrapperFunction<T, R, CONV_T2S>;
      SetColumn< T2Swrap >(pColumnNameIdx, pColumn);
    }

    template< typename T, std::string (*CONV_T2S)(const T&) >
    inline void SetColumn(const c_sizet_or_string auto& pColumnNameIdx,
                          const std::vector<T>& pColumn)  // T -> R
    {
      using T2Swrap = T2SwrapperFunction<T, T, CONV_T2S>;  //  doesn't work -> T2SwrapperFunction<T, CONV_T2S>;
      SetColumn< T2Swrap >(pColumnNameIdx, pColumn);
    }

    /**
     * @brief   Remove column either by it's index or name.
     * @param   pColumnNameIdx        column-name or zero-based column-index.
     */
    void RemoveColumn(const c_sizet_or_string auto& pColumnNameIdx)
    {
      const size_t pColumnIdx = GetColumnIdx(pColumnNameIdx);
      const ssize_t dataColumnIdx = static_cast<ssize_t>(_getDataColumnIndex(pColumnIdx).dataIdx);
      for (auto itRow = _mData.begin(); itRow != _mData.end(); ++itRow)  // + static_cast<ssize_t>(_getDataRowIndex(0).dataIdx)
      {
        itRow->erase(itRow->begin() + dataColumnIdx);
      }

      _updateColumnNames();
    }

    /**
     * @brief   Insert column at specified index.
     * @tparam  T                     'type' to converted to, using string data.
     * @tparam  CONV_T2S              conversion function (optional argument).
     * @param   pColumnIdx            zero-based column index.
     * @param   pColumn               vector of column data (optional argument).
     * @param   pColumnName           column label name (optional argument)
     */
    template< c_T2Sconverter T2Sconv >
    void InsertColumn(const size_t pColumnIdx,
                      const std::vector<typename T2Sconv::input_type>& pColumn
                                   = std::vector<typename T2Sconv::input_type>(),
                      const std::string& pColumnName = std::string())
    {
      const size_t dataColumnIdx = _getDataColumnIndex(pColumnIdx).dataIdx;

      std::vector<std::string> column;
      if (pColumn.empty())
      {
        column.resize(_getDataRowCount());
      } else {
        column.resize(_getDataRowIndex(pColumn.size()).dataIdx);
        size_t rowIdx = _getDataRowIndex(0).dataIdx;
        for (auto itCell = pColumn.begin(); itCell != pColumn.end(); ++itCell, ++rowIdx)
        {
          column.at(rowIdx) = T2Sconv::ToStr(*itCell);
        }
      }

      while (column.size() > _getDataRowCount())
      {
        t_dataRow row;
        const size_t columnCount = std::max<size_t>(_getDataRowIndex(0).dataIdx, _getDataColumnCount());
        row.resize(columnCount);
        _mData.push_back(row);
      }

      size_t rowIdx = 0;
      for (auto itRow = _mData.begin(); itRow != _mData.end(); ++itRow, ++rowIdx)
      {
        itRow->insert(itRow->begin() + static_cast<ssize_t>(dataColumnIdx), column.at(rowIdx));
      }


      if (!pColumnName.empty())
      {
        SetColumnName(pColumnIdx, pColumnName);
      }

      _updateColumnNames();
    }

    template< c_NOT_T2Sconverter T >
    inline void
    InsertColumn(const size_t pColumnIdx,
                 const std::vector<T>& pColumn = std::vector<T>(),  // T -> R
                 const std::string& pColumnName = std::string())
    {
      InsertColumn<ConvertFromVal<T>>(pColumnIdx, pColumn, pColumnName);
    }

    // TODO std::string (*CONV_T2S)(const auto&) :: auto -> R
    template<typename T,
             std::string (*CONV_T2S)(const T&) >
    inline void InsertColumn(const size_t pColumnIdx,
                             const std::vector<T>& pColumn = std::vector<T>(),  // T -> R
                             const std::string& pColumnName = std::string())
    {
      using T2Swrap = T2SwrapperFunction<T, T, CONV_T2S>;  //  doesn't work -> T2SwrapperFunction<T, CONV_T2S>;
      InsertColumn< T2Swrap >(pColumnIdx, pColumn, pColumnName);
    }

    /**
     * @brief   Get number of data columns (excluding label columns).
     * @returns column count.
     */
    size_t GetColumnCount() const
    {
      const size_t firstRowSize = ((_mData.size() > 0) ? _mData.at(0).size() : 0);
      const size_t firstDataColumnIndex = _getDataColumnIndex(0).dataIdx;
      return (firstRowSize > firstDataColumnIndex) ? (firstRowSize - firstDataColumnIndex) : 0;
    }

    constexpr size_t GetRowIdx(const size_t pRowIdx) const
    {
      return pRowIdx;
    }
    constexpr size_t GetRowIdx(const int pRowIdx) const
    {
      return static_cast<size_t>(pRowIdx);
    }
    /**
     * @brief   Get row index by name.
     * @param   pRowName              row label name.
     * @returns zero-based row index.
     */
    size_t GetRowIdx(const std::string& pRowName) const
    {
      ssize_t rowIdx = -1;
      if (_mLabelParams.mRowNameIdx >= 0)
      {
        if (_mRowNames.find(pRowName) != _mRowNames.end())
        {
          rowIdx = static_cast<ssize_t>(_mRowNames.at(pRowName)) - static_cast<ssize_t>(_getDataRowIndex(0).dataIdx);
        }
      }
      if (rowIdx < 0)
      {
        throw std::out_of_range(std::string(__RAPIDCSV_FILE__)+":"+std::to_string(__LINE__)+" row not found: " + pRowName);
      }
      return static_cast<size_t>(rowIdx);
    }

    /**
     * @brief   Get row either by it's index or name.
     * @tparam  T                     'type' to converted to, using string data.
     * @tparam  CONV_S2T              conversion function (optional argument).
     * @param   pRowNameIdx           row-name or zero-based row index.
     * @returns vector<R> of row data. By default, R is same type as T. But if
     *          CONV_S2T = &ConvertFromStr_gNaN<T>::ToVal, then R = std::variant<T, gNaN> .
     */
    // TODO function and unit tests  for ARGS...
    template< c_S2Tconverter ... S2Tconv >
    std::tuple<typename S2Tconv::return_type ...>
    GetRow(const c_sizet_or_string auto& pRowNameIdx) const
    {
      const size_t pRowIdx = GetRowIdx(pRowNameIdx);
      const size_t dataRowIdx = _getDataRowIndex(pRowIdx).dataIdx;

      assert(sizeof...(S2Tconv) <= _mData.at(dataRowIdx).size());

      size_t colIdx = _getDataColumnIndex(0).dataIdx;
      const t_dataRow& rowData = _mData.at(dataRowIdx);
      //return std::make_tuple(S2Tconv::ToVal(rowData.at(colIdx++)) ...);  on ubuntu the tuple elements order gets reversed

      //https://stackoverflow.com/questions/65261797/varadic-template-to-tuple-is-reversed
      // work around for make_tuple
      std::tuple<typename S2Tconv::return_type...> result = std::tuple<typename S2Tconv::return_type...>();
      auto write_tuple = [&rowData,&colIdx] (auto&... wrt_result) -> void
      {
        ( (wrt_result = S2Tconv::ToVal(rowData.at(colIdx++))), ... );  // comma operator ensures the element order is from left to right
      };
      std::apply(write_tuple, result);
      return result;
    }

    template< c_NOT_S2Tconverter ... T >
    inline std::tuple<typename ConvertFromStr<T>::return_type ...>
    GetRow(const c_sizet_or_string auto& pRowNameIdx) const
    {
      return GetRow<ConvertFromStr<T> ... >(pRowNameIdx);
    }

/*
    NOTE: the code below doesn't compile, nor is it a good idea to
          have two variadic lists of T's and CONV_S2T's.

          Alternatively, if needed explicitly call using template 'S2TwrapperFunction<T, CONV_S2T>'
          GetRow< S2TwrapperFunction<T, CONV_S2T> , ... >(pRowNameIdx);

          usage details refer test063.cpp

    template< typename ... T, auto ... (*CONV_S2T)(const std::string&) >
    inline std::tuple< typename std::invoke_result_t< decltype(CONV_S2T),
                                                      const std::string&
                                                   >...
                     >
    GetRow(const c_sizet_or_string auto& pRowNameIdx) const
    {
      return GetRow< S2TwrapperFunction<T, CONV_S2T>... >(pRowNameIdx);
    }
*/

    /**
     * @brief   Get row either by it's index or name.
     * @param   pRowNameIdx           row-name or zero-based row index.
     * @returns vector<std::string> of row data
     */
    inline std::vector<std::string>
    GetRow_VecStr(const c_sizet_or_string auto& pRowNameIdx) const
    {
      const size_t pRowIdx = GetRowIdx(pRowNameIdx);
      const size_t dataRowIdx = _getDataRowIndex(pRowIdx).dataIdx;
      const ssize_t colIdx = static_cast<ssize_t>(_getDataColumnIndex(0).dataIdx);
      return std::vector<std::string>( _mData.at(dataRowIdx).begin() + colIdx, _mData.at(dataRowIdx).end() );
    }

    /**
     * @brief   Set row either by it's index or name.
     * @tparam  T                     'type' to converted to, using string data.
     * @tparam  CONV_T2S              conversion function (optional argument).
     * @param   pRowNameIdx           row-name or zero-based row index.
     * @param   pRow                  vector of row data.
     */
    template< c_T2Sconverter ... T2Sconv >
    void SetRow(const c_sizet_or_string auto& pRowNameIdx,
                const std::tuple<typename T2Sconv::input_type ...>& pRow)
    {
      const size_t pRowIdx = GetRowIdx(pRowNameIdx);
      const size_t dataRowIdx = _getDataRowIndex(pRowIdx).dataIdx;
      _resizeTable(dataRowIdx, sizeof...(T2Sconv) );

      size_t colIdx = _getDataColumnIndex(0).dataIdx;
      t_dataRow& rowData = _mData.at(dataRowIdx);

      //https://stackoverflow.com/questions/42494715/c-transform-a-stdtuplea-a-a-to-a-stdvector-or-stddeque
      auto read_tuple = [&rowData,&colIdx] (auto&&... rowElem) -> void
      {
        ( (rowData.at(colIdx++) = T2Sconv::ToStr(rowElem)) , ... );
      };
      std::apply(read_tuple, pRow);
    }

    template< c_NOT_T2Sconverter ... T >
    inline void SetRow(const c_sizet_or_string auto& pRowNameIdx,
                       const std::tuple<typename ConvertFromVal<T>::input_type ...>& pRow)
    {
      SetRow< ConvertFromVal<T> ... >(pRowNameIdx, pRow);
    }

    /*
    // TODO std::string (*CONV_T2S)(const auto&) :: auto -> R
    template<typename T, std::string (*CONV_T2S)(const T&) >
    void SetRow(const c_sizet_or_string auto& pRowNameIdx,
                const std::vector<T>& pRow)
    {
      using T2Swrap = T2SwrapperFunction<T, T, CONV_T2S>;  //  doesn't work -> T2SwrapperFunction<T, CONV_T2S>;
      SetRow<T, T2Swrap >(pRowNameIdx, pRow);
    }
    */

    /**
     * @brief   Set row either by it's index or name.
     * @param   pRowNameIdx           row-name or zero-based row index.
     * @param   pRow                  vector of row data.
     */
    void SetRow_VecStr(const c_sizet_or_string auto& pRowNameIdx,
                       const std::vector<std::string>& pRow)
    {
      const size_t pRowIdx = GetRowIdx(pRowNameIdx);
      const size_t dataRowIdx = _getDataRowIndex(pRowIdx).dataIdx;
      _resizeTable(dataRowIdx, pRow.size());

      const ssize_t colIdx = static_cast<ssize_t>(_getDataColumnIndex(0).dataIdx);
      auto itCol = _mData.at(dataRowIdx).begin() + colIdx;
      for(auto itEle = pRow.begin(); itEle != pRow.end(); ++itEle, ++itCol)
      {
        (*itCol) = (*itEle);
      }
    }

    /**
     * @brief   Remove row either by it's index or name.
     * @param   pRowNameIdx           row-name or zero-based row-index.
     */
    void RemoveRow(const c_sizet_or_string auto& pRowNameIdx)
    {
      const size_t pRowIdx = GetRowIdx(pRowNameIdx);
      const ssize_t dataRowIdx = static_cast<ssize_t>(_getDataRowIndex(pRowIdx).dataIdx);
      _mData.erase(_mData.begin() + dataRowIdx);
      _updateRowNames();
    }

    /**
     * @brief   Insert row at specified index.
     * @tparam  T                     'type' to converted to, using string data.
     * @tparam  CONV_T2S              conversion function (optional argument).
     * @param   pRowIdx               zero-based row index.
     * @param   pRow                  vector of row data (optional argument).
     * @param   pRowName              row label name (optional argument).
     */
    template< c_T2Sconverter ... T2Sconv >
    void InsertRow(const size_t pRowIdx,
                   const std::tuple<typename T2Sconv::input_type ...>& pRow
                                = std::tuple<typename T2Sconv::input_type ...>(),
                   const std::string& pRowName = std::string())
    {
      const size_t rowIdx = _getDataRowIndex(pRowIdx).dataIdx;

      t_dataRow row;
      if constexpr (sizeof...(T2Sconv) == 0)
      {
        row.resize(_getDataColumnCount());
      } else {
        row.resize(_getDataColumnIndex(sizeof...(T2Sconv)).dataIdx);
        size_t colIdx = _getDataColumnIndex(0).dataIdx;

        //https://stackoverflow.com/questions/42494715/c-transform-a-stdtuplea-a-a-to-a-stdvector-or-stddeque
        auto read_tuple = [&row,&colIdx] (auto&&... rowElem) -> void
        {
          ( (row.at(colIdx++) = T2Sconv::ToStr(rowElem)) , ... );
        };
        std::apply(read_tuple, pRow);
      }

      while (rowIdx > _getDataRowCount())
      {
        t_dataRow tempRow;
        tempRow.resize(_getDataColumnCount());
        _mData.push_back(tempRow);
      }

      _mData.insert(_mData.begin() + static_cast<ssize_t>(rowIdx), row);

      if (!pRowName.empty())
      {
        SetRowName(pRowIdx, pRowName);
      }

      _updateRowNames();
    }

    template< c_NOT_T2Sconverter ... T >
    void InsertRow(const size_t pRowIdx,
                   const std::tuple<typename ConvertFromVal<T>::input_type ...>& pRow
                                = std::tuple<typename ConvertFromVal<T>::input_type ...>(),
                   const std::string& pRowName = std::string())
    {
      InsertRow< ConvertFromVal<T> ... >(pRowIdx, pRow, pRowName);
    }

    /*
    // TODO std::string (*CONV_T2S)(const auto&) :: auto -> R
    template<typename T,
             std::string (*CONV_T2S)(const T&) >
    inline
    void InsertRow(const size_t pRowIdx,
                   const std::vector<T>& pRow = std::vector<T>(),  // T -> R
                   const std::string& pRowName = std::string())
    {
      using T2Swrap = T2SwrapperFunction<T, T, CONV_T2S>;  //  doesn't work -> T2SwrapperFunction<T, CONV_T2S>;
      InsertRow<T,T2Swrap>(pRowIdx, pRow, pRowName);
    }
    */

    /**
     * @brief   Insert row at specified index.
     * @param   pRowIdx               zero-based row index.
     * @param   pRow                  vector of row data (optional argument).
     * @param   pRowName              row label name (optional argument).
     */
    void InsertRow_VecStr(const size_t pRowIdx,
                          const std::vector<std::string>& pRow
                                         = std::vector<std::string>(),
                          const std::string& pRowName = std::string())
    {
      const size_t rowIdx = _getDataRowIndex(pRowIdx).dataIdx;

      t_dataRow row;
      if (pRow.empty())
      {
        row.resize(_getDataColumnCount());
      } else {
        row.resize(_getDataColumnIndex(pRow.size()).dataIdx);
        size_t colIdx = _getDataColumnIndex(0).dataIdx;
        for (auto itCol = pRow.begin(); itCol != pRow.end(); ++itCol, ++colIdx)
        {
          row.at(colIdx) = (*itCol);
        }
      }

      while (rowIdx > _getDataRowCount())
      {
        t_dataRow tempRow;
        tempRow.resize(_getDataColumnCount());
        _mData.push_back(tempRow);
      }

      _mData.insert(_mData.begin() + static_cast<ssize_t>(rowIdx), row);

      if (!pRowName.empty())
      {
        SetRowName(pRowIdx, pRowName);
      }

      _updateRowNames();
    }

    /**
     * @brief   Get number of data rows (excluding label rows).
     * @returns row count.
     */
    size_t GetRowCount() const
    {
      const size_t dataRowSize = _mData.size();
      const size_t firstDataRowIndex = _getDataRowIndex(0).dataIdx;
      return (dataRowSize > firstDataRowIndex) ? (dataRowSize - firstDataRowIndex) : 0;
    }

    /**
     * @brief   Get cell either by it's index or name.
     * @tparam  T                     'type' to converted to, using string data.
     * @tparam  CONV_S2T              conversion function (optional argument).
     * @param   pColumnNameIdx        column-name or zero-based column-index.
     * @param   pRowNameIdx           column-name or zero-based row-index.
     * @returns cell data of type R. By default, R is same type as T. But if
     *          CONV_S2T = &ConvertFromStr_gNaN<T>::ToVal, then R = std::variant<T, gNaN> .
     */
    // TODO function and unit tests  for ARGS...
    template< typename T, c_S2Tconverter S2Tconv = ConvertFromStr<T> >
    typename S2Tconv::return_type
    GetCell(const c_sizet_or_string auto& pColumnNameIdx,
            const c_sizet_or_string auto& pRowNameIdx) const
    {
      const size_t pColumnIdx = GetColumnIdx(pColumnNameIdx);
      const size_t pRowIdx = GetRowIdx(pRowNameIdx);

      const size_t dataColumnIdx = _getDataColumnIndex(pColumnIdx).dataIdx;
      const size_t dataRowIdx = _getDataRowIndex(pRowIdx).dataIdx;

      return S2Tconv::ToVal(_mData.at(dataRowIdx).at(dataColumnIdx));
    }

    template< typename T, auto (*CONV_S2T)(const std::string&) >
    inline typename std::invoke_result_t< decltype(CONV_S2T),
                                          const std::string&
                                        >
    GetCell(const c_sizet_or_string auto& pColumnNameIdx,
            const c_sizet_or_string auto& pRowNameIdx) const
    {
      return GetCell<T, S2TwrapperFunction<T, CONV_S2T> >(pColumnNameIdx, pRowNameIdx);
    }


    /**
     * @brief   Set cell either by it's index or name.
     * @tparam  T                     'type' to converted to, using string data.
     * @tparam  CONV_S2T              conversion function (optional argument).
     * @param   pColumnNameIdx        column-name or zero-based column-index.
     * @param   pRowNameIdx           column-name or zero-based row-index.
     * @param   pCell                 cell data.
     */
    // TODO function and unit tests  for ARGS...
    template<typename T, c_T2Sconverter T2Sconv = ConvertFromVal<T> >
    void SetCell(const c_sizet_or_string auto& pColumnNameIdx,
                 const c_sizet_or_string auto& pRowNameIdx, const typename T2Sconv::input_type& pCell)
    {
      const size_t pColumnIdx = GetColumnIdx(pColumnNameIdx);
      const size_t pRowIdx = GetRowIdx(pRowNameIdx);

      const size_t dataColumnIdx = _getDataColumnIndex(pColumnIdx).dataIdx;
      const size_t dataRowIdx = _getDataRowIndex(pRowIdx).dataIdx;

      while ((dataRowIdx + 1) > _getDataRowCount())
      {
        t_dataRow row;
        row.resize(_getDataColumnCount());
        _mData.push_back(row);
      }

      if ((dataColumnIdx + 1) > _getDataColumnCount())
      {
        for (auto itRow = _mData.begin(); itRow != _mData.end(); ++itRow)  // + static_cast<ssize_t>(_getDataRowIndex(0).dataIdx)
        {
          itRow->resize(dataColumnIdx + 1);
        }
      }

      _mData.at(dataRowIdx).at(dataColumnIdx) = T2Sconv::ToStr(pCell);
    }

    // TODO std::string (*CONV_T2S)(const auto&) :: auto -> R
    template<typename T,
             std::string (*CONV_T2S)(const T&) >
    void SetCell(const c_sizet_or_string auto& pColumnNameIdx,
                 const c_sizet_or_string auto& pRowNameIdx, const T& pCell)   // T -> R
    {
      using T2Swrap = T2SwrapperFunction<T, T, CONV_T2S>;  //  doesn't work -> T2SwrapperFunction<T, CONV_T2S>;
      SetCell<T, T2Swrap >(pColumnNameIdx, pRowNameIdx, pCell);
    }

    /**
     * @brief   Get column name
     * @param   pColumnIdx            zero-based column index.
     * @returns column name.
     */
    std::string GetColumnName(const size_t pColumnIdx) const
    {
      const size_t dataColumnIdx = _getDataColumnIndex(pColumnIdx).dataIdx;
      if (_mLabelParams.mColumnNameIdx < 0)
      {
        throw std::out_of_range(std::string(__RAPIDCSV_FILE__)+":"+std::to_string(__LINE__)+" column name row index < 0: " + std::to_string(_mLabelParams.mColumnNameIdx));
      }

      return _mData.at(static_cast<size_t>(_mLabelParams.mColumnNameIdx)).at(dataColumnIdx);
    }

    /**
     * @brief   Set column name
     * @param   pColumnIdx            zero-based column index.
     * @param   pColumnName           column name.
     */
    void SetColumnName(size_t pColumnIdx, const std::string& pColumnName)
    {
      if (_mLabelParams.mColumnNameIdx < 0)
      {
        throw std::out_of_range(std::string(__RAPIDCSV_FILE__)+":"+std::to_string(__LINE__)+" column name row index < 0: " + std::to_string(_mLabelParams.mColumnNameIdx));
      }

      const size_t dataColumnIdx = _getDataColumnIndex(pColumnIdx).dataIdx;
      _mColumnNames[pColumnName] = dataColumnIdx;

      // increase table size if necessary:
      const size_t rowIdx = static_cast<size_t>(_mLabelParams.mColumnNameIdx);
      if (rowIdx >= _mData.size())
      {
        _mData.resize(rowIdx + 1);
      }
      auto& row = _mData[rowIdx];
      if (dataColumnIdx >= row.size())
      {
        row.resize(dataColumnIdx + 1);
      }

      _mData.at(static_cast<size_t>(_mLabelParams.mColumnNameIdx)).at(dataColumnIdx) = pColumnName;
    }

    /**
     * @brief   Get column names
     * @returns vector of column names.
     */
    std::vector<std::string> GetColumnNames() const
    {
      if (_mLabelParams.mColumnNameIdx >= 0)
      {
        return std::vector<std::string>(_mData.at(static_cast<size_t>(_mLabelParams.mColumnNameIdx)).begin() +
                                        static_cast<ssize_t>(_getDataColumnIndex(0).dataIdx),
                                        _mData.at(static_cast<size_t>(_mLabelParams.mColumnNameIdx)).end());
      }

      return std::vector<std::string>();
    }

    /**
     * @brief   Get row name
     * @param   pRowIdx               zero-based column index.
     * @returns row name.
     */
    std::string GetRowName(const size_t pRowIdx) const
    {
      const size_t dataRowIdx = _getDataRowIndex(pRowIdx).dataIdx;
      if (_mLabelParams.mRowNameIdx < 0)
      {
        throw std::out_of_range(std::string(__RAPIDCSV_FILE__)+":"+std::to_string(__LINE__)+" row name column index < 0: " + std::to_string(_mLabelParams.mRowNameIdx));
      }

      return _mData.at(dataRowIdx).at(static_cast<size_t>(_mLabelParams.mRowNameIdx));
    }

    /**
     * @brief   Set row name
     * @param   pRowIdx               zero-based row index.
     * @param   pRowName              row name.
     */
    void SetRowName(size_t pRowIdx, const std::string& pRowName)
    {
      const size_t dataRowIdx = _getDataRowIndex(pRowIdx).dataIdx;
      _mRowNames[pRowName] = dataRowIdx;
      if (_mLabelParams.mRowNameIdx < 0)
      {
        throw std::out_of_range(std::string(__RAPIDCSV_FILE__)+":"+std::to_string(__LINE__)+" row name column index < 0: " + std::to_string(_mLabelParams.mRowNameIdx));
      }

      // increase table size if necessary:
      if (dataRowIdx >= _mData.size())
      {
        _mData.resize(dataRowIdx + 1);
      }
      auto& row = _mData[dataRowIdx];
      if (_mLabelParams.mRowNameIdx >= static_cast<ssize_t>(row.size()))
      {
        row.resize(static_cast<size_t>(_mLabelParams.mRowNameIdx) + 1);
      }

      _mData.at(dataRowIdx).at(static_cast<size_t>(_mLabelParams.mRowNameIdx)) = pRowName;
    }

    /**
     * @brief   Get row names
     * @returns vector of row names.
     */
    std::vector<std::string> GetRowNames() const
    {
      std::vector<std::string> rownames;
      if (_mLabelParams.mRowNameIdx >= 0)
      {
        for (auto itRow = _mData.begin() + static_cast<ssize_t>(_getDataRowIndex(0).dataIdx); itRow != _mData.end(); ++itRow)
        {
          rownames.push_back(itRow->at(static_cast<size_t>(_mLabelParams.mRowNameIdx)));
        }
      }
      return rownames;
    }

  private:
    void _readCsv()
    {
      std::ifstream stream;
      stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      stream.open(_mPath, std::ios::binary);
      _readCsv(stream);
    }

    void _readCsv(std::istream& pStream)
    {
      Clear();
      pStream.seekg(0, std::ios::end);
      std::streamsize length = pStream.tellg();
      pStream.seekg(0, std::ios::beg);

#ifdef HAS_CODECVT
      std::vector<char> bom2b(2, '\0');
      if (length >= 2)
      {
        pStream.read(bom2b.data(), 2);
        pStream.seekg(0, std::ios::beg);
      }

      static const std::vector<char> bomU16le = { '\xff', '\xfe' };
      static const std::vector<char> bomU16be = { '\xfe', '\xff' };
      if ((bom2b == bomU16le) || (bom2b == bomU16be))
      {
        mIsUtf16 = true;
        mIsLE = (bom2b == bomU16le);

        std::wifstream wstream;
        wstream.exceptions(std::wifstream::failbit | std::wifstream::badbit);
        wstream.open(_mPath, std::ios::binary);
        if (mIsLE)
        {
          wstream.imbue(std::locale(wstream.getloc(),
                                    new std::codecvt_utf16<wchar_t, 0x10ffff,
                                                           static_cast<std::codecvt_mode>(std::consume_header |
                                                                                          std::little_endian)>));
        }
        else
        {
          wstream.imbue(std::locale(wstream.getloc(),
                                    new std::codecvt_utf16<wchar_t, 0x10ffff,
                                                           std::consume_header>));
        }
        std::wstringstream wss;
        wss << wstream.rdbuf();
        std::string utf8 = _toString(wss.str());
        std::stringstream ss(utf8);
        _parseCsv(ss, static_cast<std::streamsize>(utf8.size()));
      }
      else
#endif
      {
        // check for UTF-8 Byte order mark and skip it when found
        if (length >= 3)
        {
          std::vector<char> bom3b(3, '\0');
          pStream.read(bom3b.data(), 3);

          if (bom3b != s_Utf8BOM)
          {
            // file does not start with a UTF-8 Byte order mark
            pStream.seekg(0, std::ios::beg);
          }
          else
          {
            // file did start with a UTF-8 Byte order mark, simply skip it
            length -= 3;
            _mHasUtf8BOM = true;
          }
        }

        _parseCsv(pStream, length);
      }
    }

    void _parseCsv(std::istream& pStream, std::streamsize p_FileLength)
    {
      const std::streamsize bufLength = 64 * 1024;
      std::vector<char> buffer(bufLength);
      t_dataRow row;
      std::string cell;
      bool quoted = false;
      int cr = 0;
      int lf = 0;

      while (p_FileLength > 0)
      {
        const std::streamsize toReadLength = std::min<std::streamsize>(p_FileLength, bufLength);
        pStream.read(buffer.data(), toReadLength);

        // With user-specified istream opened in non-binary mode on windows, we may have a
        // data length mismatch, so ensure we don't parse outside actual data length read.
        const std::streamsize readLength = pStream.gcount();
        if (readLength <= 0)
        {
          break;
        }

        for (size_t i = 0; i < static_cast<size_t>(readLength); ++i)
        {
          if (buffer[i] == _mSeparatorParams.mQuoteChar)
          {
            if (cell.empty() || (cell[0] == _mSeparatorParams.mQuoteChar))
            {
              quoted = !quoted;
            }
            cell += buffer[i];
          }
          else if (buffer[i] == _mSeparatorParams.mSeparator)
          {
            if (!quoted)
            {
              row.push_back(_unquote(_trim(cell)));
              cell.clear();
            }
            else
            {
              cell += buffer[i];
            }
          }
          else if (buffer[i] == '\r')
          {
            if (_mSeparatorParams.mQuotedLinebreaks && quoted)
            {
              cell += buffer[i];
            }
            else
            {
              ++cr;
            }
          }
          else if (buffer[i] == '\n')
          {
            if (_mSeparatorParams.mQuotedLinebreaks && quoted)
            {
              cell += buffer[i];
            }
            else
            {
              ++lf;
              if (_mLineReaderParams.mSkipEmptyLines && row.empty() && cell.empty())
              {
                // skip empty line
              }
              else
              {
                row.push_back(_unquote(_trim(cell)));

                if (_mLineReaderParams.mSkipCommentLines && !row.at(0).empty() &&
                    (row.at(0)[0] == _mLineReaderParams.mCommentPrefix))
                {
                  // skip comment line
                }
                else
                {
                  _mData.push_back(row);
                }

                cell.clear();
                row.clear();
                quoted = false;
              }
            }
          }
          else
          {
            cell += buffer[i];
          }
        }
        p_FileLength -= readLength;
      }

      // Handle last line without linebreak
      if (!cell.empty() || !row.empty())
      {
        row.push_back(_unquote(_trim(cell)));
        cell.clear();
        _mData.push_back(row);
        row.clear();
      }

      // Assume CR/LF if at least half the linebreaks have CR
      _mSeparatorParams.mHasCR = (cr > (lf / 2));

      // Set up column labels
      _updateColumnNames();

      // Set up row labels
      _updateRowNames();
    }

    void _writeCsv() const
    {
#ifdef HAS_CODECVT
      if (mIsUtf16)
      {
        std::stringstream ss;
        _writeCsv(ss);
        std::string utf8 = ss.str();
        std::wstring wstr = _toWString(utf8);

        std::wofstream wstream;
        wstream.exceptions(std::wofstream::failbit | std::wofstream::badbit);
        wstream.open(_mPath, std::ios::binary | std::ios::trunc);

        if (mIsLE)
        {
          wstream.imbue(std::locale(wstream.getloc(),
                                    new std::codecvt_utf16<wchar_t, 0x10ffff,
                                                           static_cast<std::codecvt_mode>(std::little_endian)>));
        }
        else
        {
          wstream.imbue(std::locale(wstream.getloc(),
                                    new std::codecvt_utf16<wchar_t, 0x10ffff>));
        }

        wstream << static_cast<wchar_t>(0xfeff);
        wstream << wstr;
      }
      else
#endif
      {
        std::ofstream stream;
        stream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        stream.open(_mPath, std::ios::binary | std::ios::trunc);
        if (_mHasUtf8BOM)
        {
          stream.write(s_Utf8BOM.data(), 3);
        }

        _writeCsv(stream);
      }
    }

    void _writeCsv(std::ostream& pStream) const
    {
      for (auto itr = _mData.begin(); itr != _mData.end(); ++itr)
      {
        for (auto itc = itr->begin(); itc != itr->end(); ++itc)
        {
          if (_mSeparatorParams.mAutoQuote &&
              ((itc->find(_mSeparatorParams.mSeparator) != std::string::npos) ||
               (itc->find(' ') != std::string::npos)))
          {
            // escape quotes in string
            std::string str = *itc;
            const std::string quoteCharStr = std::string(1, _mSeparatorParams.mQuoteChar);
            _replaceString(str, quoteCharStr, quoteCharStr + quoteCharStr);

            pStream << quoteCharStr << str << quoteCharStr;
          }
          else
          {
            pStream << *itc;
          }

          if (std::distance(itc, itr->end()) > 1)
          {
            pStream << _mSeparatorParams.mSeparator;
          }
        }
        pStream << (_mSeparatorParams.mHasCR ? "\r\n" : "\n");
      }
    }

    inline size_t _getDataRowCount() const
    {
      return _mData.size();
    }

    inline size_t _getDataColumnCount() const
    {
      const size_t firstDataRow = static_cast<size_t>((_mLabelParams.mColumnNameIdx >= 0) ? _mLabelParams.mColumnNameIdx : 0);
      return (_mData.size() > firstDataRow) ? _mData.at(firstDataRow).size() : 0;
    }

    /**
     * @brief   Get actual index, taking into account header-row if present.
     * @param   pRowIdx               zero-based row index.
     * @returns zero-based row index when header-row is absent OR one-based row index when header-row is present.
     */
    inline RawIdx _getDataRowIndex(const size_t pRowIdx) const
    {
      return RawIdx{pRowIdx + static_cast<size_t>(_mLabelParams.mColumnNameIdx + 1)};
    }

    /**
     * @brief   Get actual index, taking into account header-column if present.
     * @param   pColumnIdx            zero-based column index.
     * @returns zero-based column index when header-column is absent OR one-based column index when header-column is present.
     */
    inline RawIdx _getDataColumnIndex(const size_t pColumnIdx) const
    {
      return RawIdx{pColumnIdx + static_cast<size_t>(_mLabelParams.mRowNameIdx + 1)};
    }

    std::string _trim(const std::string& pStr) const
    {
      if (_mSeparatorParams.mTrim)
      {
        std::string str = pStr;

        // ltrim
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) { return !isspace(ch); }));

        // rtrim
        str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) { return !isspace(ch); }).base(), str.end());

        return str;
      }
      else
      {
        return pStr;
      }
    }

    std::string _unquote(const std::string& pStr) const
    {
      if (_mSeparatorParams.mAutoQuote && (pStr.size() >= 2) &&
          (pStr.front() == _mSeparatorParams.mQuoteChar) &&
          (pStr.back() == _mSeparatorParams.mQuoteChar))
      {
        // remove start/end quotes
        std::string str = pStr.substr(1, pStr.size() - 2);

        // unescape quotes in string
        const std::string quoteCharStr = std::string(1, _mSeparatorParams.mQuoteChar);
        _replaceString(str, quoteCharStr + quoteCharStr, quoteCharStr);

        return str;
      }
      else
      {
        return pStr;
      }
    }

    void _updateColumnNames()
    {
      _mColumnNames.clear();
      if ((_mLabelParams.mColumnNameIdx >= 0) &&
          (static_cast<ssize_t>(_mData.size()) > _mLabelParams.mColumnNameIdx))
      {
        size_t i = 0;
        for (auto& columnName : _mData[static_cast<size_t>(_mLabelParams.mColumnNameIdx)])
        {
          _mColumnNames[columnName] = i++;
        }
      }
    }

    void _updateRowNames()
    {
      _mRowNames.clear();
      if ((_mLabelParams.mRowNameIdx >= 0) && (_mData.size() > _getDataRowIndex(0).dataIdx))
      {
        size_t i = 0;
        for (auto& dataRow : _mData)
        {
          if (static_cast<ssize_t>(dataRow.size()) > _mLabelParams.mRowNameIdx)
          {
            _mRowNames[dataRow[static_cast<size_t>(_mLabelParams.mRowNameIdx)]] = i++;
          }
        }
      }
    }

#ifdef HAS_CODECVT
#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable: 4996)
#endif
    static std::string _toString(const std::wstring& pWStr)
    {
      return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>{ }.to_bytes(pWStr);
    }

    static std::wstring _toWString(const std::string& pStr)
    {
      return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>{ }.from_bytes(pStr);
    }
#if defined(_MSC_VER)
#pragma warning (pop)
#endif
#endif

    static void _replaceString(std::string& pStr, const std::string& pSearch, const std::string& pReplace)
    {
      size_t pos = 0;

      while ((pos = pStr.find(pSearch, pos)) != std::string::npos)
      {
        pStr.replace(pos, pSearch.size(), pReplace);
        pos += pReplace.size();
      }
    }

    std::string _mPath;
    LabelParams _mLabelParams;
    SeparatorParams _mSeparatorParams;
    LineReaderParams _mLineReaderParams;
    std::vector<t_dataRow> _mData;
    std::map<std::string, size_t> _mColumnNames;
    std::map<std::string, size_t> _mRowNames;
#ifdef HAS_CODECVT
    bool mIsUtf16 = false;
    bool mIsLE = false;
#endif
    bool _mHasUtf8BOM = false;


    void _resizeTable(const size_t dataRowIdx, const size_t pRowSize)
    {
      while ((dataRowIdx + 1) > _getDataRowCount())
      {
        t_dataRow row;
        row.resize(_getDataColumnCount());
        _mData.push_back(row);
      }

      if (pRowSize > _getDataColumnCount())
      {
        for (auto itRow = _mData.begin(); itRow != _mData.end(); ++itRow)  // + static_cast<ssize_t>(_getDataRowIndex(0).dataIdx)
        {
          itRow->resize(_getDataColumnIndex(pRowSize).dataIdx);
        }
      }
    }

    friend class _ViewDocument;
  };
}
