// Copyright (c) 2017-2020, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

/*!
 *******************************************************************************
 * \file JSONReader.hpp
 *
 * \brief This file contains the class definition of the JSONReader.
 *******************************************************************************
 */

#ifndef INLET_JSONREADER_HPP
#define INLET_JSONREADER_HPP

#include "axom/inlet/Reader.hpp"

#include "conduit.hpp"

namespace axom
{
namespace inlet
{
/*!
 *******************************************************************************
 * \class JSONReader
 *
 * \brief A Reader that is able to read variables from a JSON file.
 *
 * \see Reader
 *******************************************************************************
 */
class JSONReader : public Reader
{
public:
  JSONReader();

  /*!
   *****************************************************************************
   * \brief Parses the given input file.
   *
   * This performs any setup work and parses the given input file.
   * It is required that this is called before using the Reader and overrides
   * any JSON state that was previously there.
   *
   * \param [in] filePath The Input file to be read
   *
   * \return true if the input file was able to be parsed
   *****************************************************************************
   */
  bool parseFile(const std::string& filePath);

  /*!
   *****************************************************************************
   * \brief Parses the given JSON string.
   *
   * This performs any setup work and parses the given JSON string.
   * It is required that this is called before using the Reader and overrides
   * any JSON state that was previously there.
   *
   * \param [in] JSONString The Input file to be read
   *
   * \return true if the string was able to be parsed
   *****************************************************************************
   */
  bool parseString(const std::string& JSONString);

  /*!
   *****************************************************************************
   * \brief Return a boolean out of the input file
   *
   * This performs any necessary retrieval and mapping from the given identifier
   * to what is in the input file.
   *
   * \param [in]  id    The identifier to the bool that will be retrieved
   * \param [out] value The value of the bool that was retrieved
   *
   * \return true if the variable was able to be retrieved from the file
   *****************************************************************************
   */
  bool getBool(const std::string& id, bool& value);

  /*!
   *****************************************************************************
   * \brief Return a double out of the input file
   *
   * This performs any necessary retrieval and mapping from the given identifier
   * to what is in the input file.
   *
   * \param [in]  id    The identifier to the double that will be retrieved
   * \param [out] value The value of the double that was retrieved
   *
   * \return true if the variable was able to be retrieved from the file
   *****************************************************************************
   */
  bool getDouble(const std::string& id, double& value);

  /*!
   *****************************************************************************
   * \brief Return a int out of the input file
   *
   * This performs any necessary retrieval and mapping from the given identifier
   * to what is in the input file.
   *
   * \param [in]  id    The identifier to the int that will be retrieved from
   *the file
   * \param [out] value The value of the int that was retrieved from the file
   *
   * \return true if the variable was able to be retrieved from the file
   *****************************************************************************
   */
  bool getInt(const std::string& id, int& value);

  /*!
   *****************************************************************************
   * \brief Return a string out of the input file
   *
   * This performs any necessary retrieval and mapping from the given identifier
   * to what is in the input file.
   *
   * \param [in]  id    The identifier to the string that will be retrieved
   * \param [out] value The value of the string that was retrieved
   *
   * \return true if the variable was able to be retrieved from the file
   *****************************************************************************
   */
  bool getString(const std::string& id, std::string& value);

  /*!
   *****************************************************************************
   * \brief Get an index-integer mapping for the given JSON array
   *
   * This performs any necessary retrieval and mapping from the given identifier
   * to what is in the input file.
   *
   * \param [in]  id    The identifier to the string that will be retrieved
   * \param [out] map The values of the ints that were retrieved
   *
   * \return true if the array was able to be retrieved from the file
   *****************************************************************************
   */
  bool getIntMap(const std::string& id, std::unordered_map<int, int>& values);
  bool getIntMap(const std::string& id,
                 std::unordered_map<std::string, int>& values);

  /*!
   *****************************************************************************
   * \brief Get an index-double mapping for the given JSON array
   *
   * This performs any necessary retrieval and mapping from the given identifier
   * to what is in the input file.
   *
   * \param [in]  id    The identifier to the string that will be retrieved
   * \param [out] map The values of the doubles that were retrieved
   *
   * \return true if the array was able to be retrieved from the file
   *****************************************************************************
   */
  bool getDoubleMap(const std::string& id,
                    std::unordered_map<int, double>& values);
  bool getDoubleMap(const std::string& id,
                    std::unordered_map<std::string, double>& values);

  /*!
   *****************************************************************************
   * \brief Get an index-bool mapping for the given JSON array
   *
   * This performs any necessary retrieval and mapping from the given identifier
   * to what is in the input file.
   *
   * \param [in]  id    The identifier to the string that will be retrieved
   * \param [out] map The values of the bools that were retrieved
   *
   * \return true if the array was able to be retrieved from the file
   *****************************************************************************
   */
  bool getBoolMap(const std::string& id, std::unordered_map<int, bool>& values);
  bool getBoolMap(const std::string& id,
                  std::unordered_map<std::string, bool>& values);

  /*!
   *****************************************************************************
   * \brief Get an index-string mapping for the given JSON array
   *
   * This performs any necessary retrieval and mapping from the given identifier
   * to what is in the input file.
   *
   * \param [in]  id    The identifier to the string that will be retrieved
   * \param [out] map The values of the strings that were retrieved
   *
   * \return true if the array was able to be retrieved from the file
   *****************************************************************************
   */
  bool getStringMap(const std::string& id,
                    std::unordered_map<int, std::string>& values);
  bool getStringMap(const std::string& id,
                    std::unordered_map<std::string, std::string>& values);

  /*!
   *****************************************************************************
   * \brief Get the list of indices for an container
   *
   * \param [in]  id    The identifier to the container that will be retrieved
   * \param [out] map The values of the indices that were retrieved
   *
   * \return true if the indices were able to be retrieved from the file
   *****************************************************************************
   */
  bool getIndices(const std::string& id, std::vector<int>& indices);
  bool getIndices(const std::string& id, std::vector<std::string>& indices);

  /*!
   *****************************************************************************
   * \brief Get a function from the input file
   *
   * \param [in]  id    The identifier to the function that will be retrieved
   * \param [in]  ret_type    The return type of the function
   * \param [in]  arg_types    The argument types of the function
   *
   * \return The function, compares false if not found
   * \note JSON does not support functions - calling this function will result
   * in a SLIC_ERROR
   *****************************************************************************
   */
  FunctionVariant getFunction(const std::string& id,
                              const FunctionType ret_type,
                              const std::vector<FunctionType>& arg_types);

  /*!
   *****************************************************************************
   * \brief The base index for arrays in Lua
   *****************************************************************************
   */
  static const int baseIndex = 0;

private:
  bool getValue(const conduit::Node& node, int& value);
  bool getValue(const conduit::Node& node, std::string& value);
  bool getValue(const conduit::Node& node, double& value);
  bool getValue(const conduit::Node& node, bool& value);

  template <typename T>
  bool getDictionary(const std::string& id,
                     std::unordered_map<std::string, T>& values);

  template <typename T>
  bool getArray(const std::string& id, std::unordered_map<int, T>& values);
  conduit::Node m_root;
};

}  // end namespace inlet
}  // end namespace axom

#endif
