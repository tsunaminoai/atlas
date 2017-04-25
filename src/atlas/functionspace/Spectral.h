/*
 * (C) Copyright 1996-2017 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#pragma once

#include "atlas/library/config.h"
#include "atlas/functionspace/FunctionSpace.h"
#include "atlas/util/Config.h"
#include "atlas/field/Field.h"

namespace atlas {
    class Field;
    class FieldSet;
}

namespace atlas {
namespace trans {
    class Trans;
}
}

namespace atlas {
namespace functionspace {
namespace detail {

// -------------------------------------------------------------------

class Spectral : public FunctionSpaceImpl
{
public:

  Spectral(const size_t truncation);

  Spectral(trans::Trans& );

  virtual ~Spectral();

  virtual std::string name() const { return "Spectral"; }

  /// @brief Create a spectral field
  template <typename DATATYPE> Field createField(
    const std::string& name,
    const eckit::Parametrisation& = util::NoConfig() ) const;
  template <typename DATATYPE> Field createField(
    const std::string& name,
    size_t levels,
    const eckit::Parametrisation& = util::NoConfig() ) const;

  void gather( const FieldSet&, FieldSet& ) const;
  void gather( const Field&,    Field& ) const;

  void scatter( const FieldSet&, FieldSet& ) const;
  void scatter( const Field&,    Field& ) const;

  std::string checksum( const FieldSet& ) const;
  std::string checksum( const Field& ) const;

  void norm( const Field&, double& norm, int rank=0 ) const;
  void norm( const Field&, double norm_per_level[], int rank=0 ) const;
  void norm( const Field&, std::vector<double>& norm_per_level, int rank=0 ) const;

public: // methods

  size_t nb_spectral_coefficients() const;
  size_t nb_spectral_coefficients_global() const;

private: // methods

  size_t config_size(const eckit::Parametrisation& config) const;
  size_t footprint() const;

private: // data

  size_t truncation_;

  trans::Trans* trans_;

};

} // detail

// -------------------------------------------------------------------

class Spectral : public FunctionSpace {
public:

  Spectral( const FunctionSpace& );
  Spectral( const size_t truncation );
  Spectral( trans::Trans& );


  operator bool() const { return valid(); }
  bool valid() const { return functionspace_; }

  /// @brief Create a spectral field
  template <typename DATATYPE> Field createField(
    const std::string& name,
    const eckit::Parametrisation& = util::NoConfig() ) const;
  template <typename DATATYPE> Field createField(
    const std::string& name,
    size_t levels,
    const eckit::Parametrisation& = util::NoConfig() ) const;

  void gather( const FieldSet&, FieldSet& ) const;
  void gather( const Field&,    Field& ) const;

  void scatter( const FieldSet&, FieldSet& ) const;
  void scatter( const Field&,    Field& ) const;

  std::string checksum( const FieldSet& ) const;
  std::string checksum( const Field& ) const;

  void norm( const Field&, double& norm, int rank=0 ) const;
  void norm( const Field&, double norm_per_level[], int rank=0 ) const;
  void norm( const Field&, std::vector<double>& norm_per_level, int rank=0 ) const;

  size_t nb_spectral_coefficients() const;
  size_t nb_spectral_coefficients_global() const;

private:

  const detail::Spectral* functionspace_;
};


// -------------------------------------------------------------------
// C wrapper interfaces to C++ routines
extern "C"
{
  const detail::Spectral* atlas__SpectralFunctionSpace__new__truncation (int truncation);
  const detail::Spectral* atlas__SpectralFunctionSpace__new__trans (trans::Trans* trans);
  void atlas__SpectralFunctionSpace__delete (detail::Spectral* This);
  field::FieldImpl* atlas__fs__Spectral__create_field_name_kind(const detail::Spectral* This, const char* name, int kind, const eckit::Parametrisation* options);
  field::FieldImpl* atlas__fs__Spectral__create_field_name_kind_lev(const detail::Spectral* This, const char* name, int kind, int levels, const eckit::Parametrisation* options);
  void atlas__SpectralFunctionSpace__gather(const detail::Spectral* This, const field::FieldImpl* local, field::FieldImpl* global);
  void atlas__SpectralFunctionSpace__gather_fieldset(const detail::Spectral* This, const field::FieldSetImpl* local, field::FieldSetImpl* global);
  void atlas__SpectralFunctionSpace__scatter(const detail::Spectral* This, const field::FieldImpl* global, field::FieldImpl* local);
  void atlas__SpectralFunctionSpace__scatter_fieldset(const detail::Spectral* This, const field::FieldSetImpl* global, field::FieldSetImpl* local);
  void atlas__SpectralFunctionSpace__norm(const detail::Spectral* This, const field::FieldImpl* field, double norm[], int rank);
}

} // namespace functionspace
} // namespace atlas
