/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "eckit/exception/Exceptions.h"

#include "atlas/field/Field.h"
#include "atlas/field/FieldSet.h"
#include "atlas/grid/Grid.h"
#include "atlas/runtime/ErrorHandling.h"

namespace atlas {
namespace field {

//------------------------------------------------------------------------------------------------------

FieldSetImpl::FieldSetImpl( const std::string& /*name*/ ) : name_() {}

void FieldSetImpl::clear() {
    index_.clear();
    fields_.clear();
}

Field FieldSetImpl::add( const Field& field ) {
    if ( field.name().size() ) { index_[field.name()] = fields_.size(); }
    else {
        std::stringstream name;
        name << name_ << "[" << fields_.size() << "]";
        index_[name.str()] = fields_.size();
    }
    fields_.push_back( field );
    return field;
}

bool FieldSetImpl::has_field( const std::string& name ) const {
    return index_.count( name );
}

Field& FieldSetImpl::field( const std::string& name ) const {
    if ( !has_field( name ) ) {
        const std::string msg( "FieldSet" + ( name_.length() ? " \"" + name_ + "\"" : "" ) + ": cannot find field \"" +
                               name + "\"" );
        throw eckit::OutOfRange( msg, Here() );
    }
    return const_cast<Field&>( fields_[index_.at( name )] );
}

void FieldSetImpl::haloExchange( bool on_device ) const {
    for ( idx_t i = 0; i < size(); ++i ) {
        field( i ).haloExchange( on_device );
    }
}

void FieldSetImpl::set_dirty( bool value ) const {
    for ( idx_t i = 0; i < size(); ++i ) {
        field( i ).set_dirty( value );
    }
}

void FieldSetImpl::throw_OutOfRange( idx_t index, idx_t max ) {
    throw eckit::OutOfRange( index, max, Here() );
}

std::vector<std::string> FieldSetImpl::field_names() const {
    std::vector<std::string> ret;

    for ( const_iterator field = cbegin(); field != cend(); ++field )
        ret.push_back( field->name() );

    return ret;
}

//-----------------------------------------------------------------------------
// C wrapper interfaces to C++ routines
extern "C" {

FieldSetImpl* atlas__FieldSet__new( char* name ) {
    ATLAS_ERROR_HANDLING( FieldSetImpl* fset = new FieldSetImpl( std::string( name ) ); fset->name() = name;
                          return fset; );
    return nullptr;
}

void atlas__FieldSet__delete( FieldSetImpl* This ) {
    ATLAS_ERROR_HANDLING( ASSERT( This != nullptr ); delete This; );
}

void atlas__FieldSet__add_field( FieldSetImpl* This, FieldImpl* field ) {
    ATLAS_ERROR_HANDLING( ASSERT( This != nullptr ); This->add( field ); );
}

int atlas__FieldSet__has_field( const FieldSetImpl* This, char* name ) {
    ATLAS_ERROR_HANDLING( ASSERT( This != nullptr ); return This->has_field( std::string( name ) ); );
    return 0;
}

idx_t atlas__FieldSet__size( const FieldSetImpl* This ) {
    ATLAS_ERROR_HANDLING( ASSERT( This != nullptr ); return This->size(); );
    return 0;
}

FieldImpl* atlas__FieldSet__field_by_name( FieldSetImpl* This, char* name ) {
    ATLAS_ERROR_HANDLING( ASSERT( This != nullptr ); return This->field( std::string( name ) ).get(); );
    return nullptr;
}

FieldImpl* atlas__FieldSet__field_by_idx( FieldSetImpl* This, idx_t idx ) {
    ATLAS_ERROR_HANDLING( ASSERT( This != nullptr ); return This->operator[]( idx ).get(); );
    return nullptr;
}

void atlas__FieldSet__set_dirty( FieldSetImpl* This, int value ) {
    ATLAS_ERROR_HANDLING( ASSERT( This != nullptr ); return This->set_dirty( value ); );
}

void atlas__FieldSet__halo_exchange( FieldSetImpl* This, int on_device ) {
    ATLAS_ERROR_HANDLING( ASSERT( This != nullptr ); return This->haloExchange( on_device ); );
}
}
//-----------------------------------------------------------------------------

}  // namespace field

//------------------------------------------------------------------------------------------------------

FieldSet::FieldSet( const std::string& name ) : Handle( new Implementation( name ) ) {}

FieldSet::FieldSet( const Field& field ) : Handle( new Implementation() ) {
    get()->add( field );
}

void FieldSet::set_dirty( bool value ) const {
    get()->set_dirty( value );
}

//------------------------------------------------------------------------------------------------------

}  // namespace atlas
