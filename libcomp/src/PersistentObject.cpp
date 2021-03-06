/**
 * @file libcomp/src/PersistentObject.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Derived class from Object and base class for peristed objgen objects.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "PersistentObject.h"

#ifndef EXOTIC_PLATFORM

// libcomp Includes
#include "BaseLog.h"
#include "BaseScriptEngine.h"
#include "Database.h"
#include "DatabaseBind.h"
#include "MetaObjectXmlParser.h"
#include "MetaVariable.h"

using namespace libcomp;

std::unordered_map<std::string, std::weak_ptr<PersistentObject>>
    PersistentObject::sCached;
std::mutex PersistentObject::mCacheLock;
PersistentObject::TypeMap PersistentObject::sTypeMap;
std::unordered_map<std::string, size_t> PersistentObject::sTypeNames;
std::unordered_map<size_t, std::function<PersistentObject*()>>
    PersistentObject::sFactory;

PersistentObject::PersistentObject(Convert::Encoding_t encoding)
    : Object(encoding), mUUID(), mDirtyFields(), mDeleted(false) {}

PersistentObject::PersistentObject(const PersistentObject& other)
    : Object(), mUUID(), mDirtyFields(), mDeleted(false) {
  (void)other;

  mSelf = std::weak_ptr<PersistentObject>();
}

PersistentObject::~PersistentObject() {
  if (!mUUID.IsNull() && !IsDeleted()) {
    std::lock_guard<std::mutex> lock(mCacheLock);

    std::string strUUID = mUUID.ToString();
    if (sCached.find(strUUID) != sCached.end()) {
      sCached.erase(strUUID);
    }
  }
}

libobjgen::UUID PersistentObject::GetUUID() const { return mUUID; }

bool PersistentObject::Register(const std::shared_ptr<PersistentObject>& self,
                                const libobjgen::UUID& pUuid) {
  if (!self->IsDeleted()) {
    bool registered = false;

    libobjgen::UUID& uuid = self->mUUID;

    std::lock_guard<std::mutex> lock(mCacheLock);
    if (!pUuid.IsNull() && !uuid.IsNull()) {
      // Unregister old UUID, keep if making a copy
      auto it = sCached.find(uuid.ToString());
      if (it != sCached.end() && it->second.lock() == self) {
        sCached.erase(it);
      }
    }

    if (!pUuid.IsNull()) {
      uuid = pUuid;
    } else if (uuid.IsNull()) {
      uuid = libobjgen::UUID::Random();
      registered = true;
    }

    if (!registered && sCached.find(uuid.ToString()) == sCached.end()) {
      registered = true;
    }

    std::string uuidString = uuid.ToString();

    if (registered) {
      self->mSelf = self;
      sCached[uuidString] = self;

      return true;
    } else {
      LogGeneralError([&]() {
        return String("Duplicate object detected: %1\n").Arg(uuidString);
      });
    }
  }

  return false;
}

void PersistentObject::Unregister() {
  mDeleted = true;

  std::lock_guard<std::mutex> lock(mCacheLock);

  auto iter = sCached.find(mUUID.ToString());
  if (iter != sCached.end()) {
    sCached.erase(iter);
  }
}

bool PersistentObject::IsDeleted() { return mDeleted; }

std::shared_ptr<PersistentObject> PersistentObject::GetObjectByUUID(
    const libobjgen::UUID& uuid) {
  std::lock_guard<std::mutex> lock(mCacheLock);

  auto iter = sCached.find(uuid.ToString());
  if (iter != sCached.end()) {
    return iter->second.lock();
  }

  return nullptr;
}

std::shared_ptr<PersistentObject> PersistentObject::LoadObjectByUUID(
    size_t typeHash, const std::shared_ptr<Database>& db,
    const libobjgen::UUID& uuid, bool reload, bool reportError) {
  auto obj = !reload ? GetObjectByUUID(uuid) : nullptr;

  if (nullptr == obj) {
    auto bind = new DatabaseBindUUID("UID", uuid);

    obj = LoadObject(typeHash, db, bind);

    delete bind;

    if (reportError && nullptr == obj) {
      LogGeneralError([&]() {
        return String("Unknown UUID '%1' for '%2' failed to load\n")
            .Arg(uuid.ToString())
            .Arg(sTypeMap[typeHash]->GetName());
      });
    }
  }

  return obj;
}

std::shared_ptr<PersistentObject> PersistentObject::LoadObject(
    size_t typeHash, const std::shared_ptr<Database>& db,
    DatabaseBind* pValue) {
  std::shared_ptr<PersistentObject> obj;

  if (nullptr != db) {
    obj = db->LoadSingleObject(typeHash, pValue);
  }

  return obj;
}

std::shared_ptr<PersistentObject> PersistentObject::LoadObject(
    size_t typeHash, const std::shared_ptr<Database>& db) {
  return LoadObject(typeHash, db, nullptr);
}

std::list<std::shared_ptr<PersistentObject>> PersistentObject::LoadObjects(
    size_t typeHash, const std::shared_ptr<Database>& db,
    DatabaseBind* pValue) {
  if (nullptr != db) {
    return db->LoadObjects(typeHash, pValue);
  }

  return std::list<std::shared_ptr<PersistentObject>>();
}

std::list<std::shared_ptr<PersistentObject>> PersistentObject::LoadObjects(
    size_t typeHash, const std::shared_ptr<Database>& db) {
  return LoadObjects(typeHash, db, nullptr);
}

void PersistentObject::RegisterType(
    std::type_index type, const std::shared_ptr<libobjgen::MetaObject>& obj,
    const std::function<PersistentObject*()>& f) {
  size_t typeHash = type.hash_code();

  sTypeMap[typeHash] = obj;
  sTypeNames[obj->GetName()] = typeHash;
  sFactory[typeHash] = f;
}

PersistentObject::TypeMap& PersistentObject::GetRegistry() { return sTypeMap; }

size_t PersistentObject::GetTypeHashByName(const std::string& name,
                                           bool& result) {
  auto iter = sTypeNames.find(name);
  result = iter != sTypeNames.end();
  return result ? iter->second : 0;
}

size_t PersistentObject::GetTypeHashByName(const std::string& name) {
  bool result;

  return GetTypeHashByName(name, result);
}

const std::shared_ptr<libobjgen::MetaObject>
PersistentObject::GetRegisteredMetadata(size_t typeHash) {
  auto iter = sTypeMap.find(typeHash);
  return iter != sTypeMap.end() ? iter->second : nullptr;
}

std::shared_ptr<libobjgen::MetaObject> PersistentObject::GetMetadataFromBytes(
    const char* bytes, size_t length) {
  if (length == 0) {
    return nullptr;
  }

  std::string definition(bytes, length);
  std::stringstream ss(definition);

  auto obj =
      std::shared_ptr<libobjgen::MetaObject>(new libobjgen::MetaObject());
  if (!obj->Load(ss)) {
    obj = nullptr;
  }

  return obj;
}

std::shared_ptr<PersistentObject> PersistentObject::New(size_t typeHash) {
  auto iter = sFactory.find(typeHash);
  return iter != sFactory.end()
             ? std::shared_ptr<PersistentObject>(iter->second())
             : nullptr;
}

bool PersistentObject::Insert(const std::shared_ptr<Database>& db) {
  if (mSelf.use_count() > 0) {
    auto self = mSelf.lock();
    return nullptr != db ? db->InsertSingleObject(self) : false;
  }

  return false;
}

bool PersistentObject::Update(const std::shared_ptr<Database>& db) {
  if (mSelf.use_count() > 0) {
    auto self = mSelf.lock();
    return nullptr != db ? db->UpdateSingleObject(self) : false;
  }

  return false;
}

bool PersistentObject::Delete(const std::shared_ptr<Database>& db) {
  if (mSelf.use_count() > 0) {
    auto self = mSelf.lock();
    return nullptr == db || db->DeleteSingleObject(self);
  }

  return false;
}

bool PersistentObject::SaveWithUUID(tinyxml2::XMLDocument& doc,
                                    tinyxml2::XMLElement& root,
                                    bool append) const {
  bool result = Save(doc, root, append);

  if (result) {
    tinyxml2::XMLElement* pMember = doc.NewElement("member");
    pMember->SetAttribute("name", "UUID");
    pMember->InsertEndChild(doc.NewText(GetUUID().ToString().c_str()));

    tinyxml2::XMLElement* pElement = root.LastChild()->ToElement();
    pElement->InsertFirstChild(pMember);
  }

  return result;
}

namespace libcomp {
template <>
BaseScriptEngine& BaseScriptEngine::Using<libobjgen::UUID>() {
  if (!BindingExists("UUID")) {
    Sqrat::Class<libobjgen::UUID> binding(mVM, "UUID");
    binding.Func("ToString", &libobjgen::UUID::ToString)
        .Func("IsNull", &libobjgen::UUID::IsNull);  // Last call to binding

    Bind<libobjgen::UUID>("UUID", binding);
  }

  return *this;
}

template <>
BaseScriptEngine& BaseScriptEngine::Using<PersistentObject>() {
  if (!BindingExists("PersistentObject")) {
    // Include the base class
    Using<Object>();

    Sqrat::DerivedClass<PersistentObject, Object,
                        Sqrat::NoConstructor<PersistentObject>>
        binding(mVM, "PersistentObject");
    Bind<PersistentObject>("PersistentObject", binding);

    // These are needed for some methods.
    Using<libobjgen::UUID>();
    Using<Database>();

    binding.Func("GetUUID", &PersistentObject::GetUUID)
        .Func("Insert", &PersistentObject::Insert)
        .Func("Update", &PersistentObject::Update)
        .Func("Delete", &PersistentObject::Delete)
        .StaticFunc("Register", &PersistentObject::Register)
        .StaticFunc<std::shared_ptr<PersistentObject> (*)(
            size_t, const std::shared_ptr<Database>&, const libobjgen::UUID&,
            bool, bool)>("LoadObjectByUUID",
                         &PersistentObject::LoadObjectByUUID)
        .StaticFunc<std::list<std::shared_ptr<PersistentObject>> (*)(
            size_t, const std::shared_ptr<Database>&)>(
            "LoadObjects", &PersistentObject::LoadObjects)
        .StaticFunc<size_t (*)(const std::string&)>(
            "GetTypeHashByName",
            &PersistentObject::GetTypeHashByName);  // Last call to binding
  }

  return *this;
}
}  // namespace libcomp

bool PersistentObject::sInitializationFailed = false;

bool PersistentObject::InitializationFailed() { return sInitializationFailed; }

#endif  // !EXOTIC_PLATFORM
