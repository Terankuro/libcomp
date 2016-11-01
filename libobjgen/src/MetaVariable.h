/**
 * @file libobjgen/src/MetaVariable.h
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Meta data for an object member variable.
 *
 * This file is part of the COMP_hack Object Generator Library (libobjgen).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBOBJGEN_SRC_METAVARIABLE_H
#define LIBOBJGEN_SRC_METAVARIABLE_H

// libobjgen Includes
#include "UUID.h"

// Standard C++11 Includes
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

// tinyxml2 Includes
#include "PushIgnore.h"
#include <tinyxml2.h>
#include "PopIgnore.h"

namespace libobjgen
{

class Generator;
class MetaObject;

class MetaVariable
{
public:
    enum class MetaVariableType_t
    {
        TYPE_S8,
        TYPE_U8,
        TYPE_S16,
        TYPE_U16,
        TYPE_S32,
        TYPE_U32,
        TYPE_S64,
        TYPE_U64,
        TYPE_FLOAT,
        TYPE_DOUBLE,
        TYPE_STRING,
        TYPE_ARRAY,
        TYPE_LIST,
        TYPE_REF,
    };

    MetaVariable();
    virtual ~MetaVariable() { }

    std::string GetName() const;
    void SetName(const std::string& name);

    std::string GetError() const;

    virtual size_t GetSize() const = 0;

    virtual MetaVariableType_t GetMetaType() const = 0;

    virtual std::string GetType() const = 0;

    virtual bool IsCoreType() const = 0;
    virtual bool IsValid() const = 0;

    virtual bool IsCaps() const;
    virtual void SetCaps(bool caps);

    virtual bool IsInherited() const;
    virtual void SetInherited(const bool inherited);

    virtual bool IsValid(const std::vector<char>& data) const;
    virtual bool IsValid(const void *pData, size_t dataSize) const = 0;

    virtual bool Load(std::istream& stream) = 0;
    virtual bool Save(std::ostream& stream) const = 0;

    virtual bool Load(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement& root) = 0;
    virtual bool Save(tinyxml2::XMLDocument& doc,
        tinyxml2::XMLElement& root) const = 0;

    virtual uint16_t GetDynamicSizeCount() const;

    virtual std::string GetCodeType() const = 0;
    virtual std::string GetConstructValue() const = 0;
    virtual std::string GetValidCondition(const Generator& generator,
        const std::string& name, bool recursive = false) const = 0;
    virtual std::string GetLoadCode(const Generator& generator,
        const std::string& name, const std::string& stream) const = 0;
    virtual std::string GetSaveCode(const Generator& generator,
        const std::string& name, const std::string& stream) const = 0;
    virtual std::string GetLoadRawCode(const Generator& generator,
        const std::string& name, const std::string& stream) const = 0;
    virtual std::string GetSaveRawCode(const Generator& generator,
        const std::string& name, const std::string& stream) const = 0;
    virtual std::string GetXmlLoadCode(const Generator& generator,
        const std::string& name, const std::string& doc,
        const std::string& root, const std::string& members,
        size_t tabLevel = 1) const = 0;
    virtual std::string GetXmlSaveCode(const Generator& generator,
        const std::string& name, const std::string& doc,
        const std::string& root, size_t tabLevel = 1) const = 0;
    virtual std::string GetDeclaration(const std::string& name) const;
    virtual std::string GetArgument(const std::string& name) const;
    virtual std::string GetGetterCode(const Generator& generator,
        const std::string& name, size_t tabLevel = 1) const;
    virtual std::string GetInternalGetterCode(const Generator& generator,
        const std::string& name) const;
    virtual std::string GetSetterCode(const Generator& generator,
        const std::string& name, const std::string argument,
        size_t tabLevel = 1) const;
    virtual std::string GetAccessDeclarations(const Generator& generator,
        const MetaObject& object, const std::string& name,
        size_t tabLevel = 1) const;
    virtual std::string GetAccessFunctions(const Generator& generator,
        const MetaObject& object, const std::string& name) const;
    virtual std::string GetConstructorCode(const Generator& generator,
        const MetaObject& object, const std::string& name,
        size_t tabLevel = 1) const;
    virtual std::string GetDestructorCode(const Generator& generator,
        const MetaObject& object, const std::string& name,
        size_t tabLevel = 1) const;
    virtual std::string GetDynamicSizeCountCode(const Generator& generator,
        const std::string& name) const;

protected:
    virtual bool BaseLoad(const tinyxml2::XMLElement& element);
    virtual bool BaseSave(tinyxml2::XMLElement& element) const;

    std::string mError;

private:
    UUID mUUID;
    bool mCaps;
    std::string mName;
    bool mInherited;
};

} // namespace libobjgen

#endif // LIBOBJGEN_SRC_METAVARIABLE_H
