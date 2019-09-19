// Copyright 2019 Adrien Guinet <adrien@guinet.me>
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/SetVector.h>
#include <dffi/composite_type.h>
#include <unordered_set>

#include "dffi_impl.h"

using namespace llvm;

namespace dffi {

void CompositeType::inlineAnonymousMembers()
{
  SmallVector<FieldsMapTy::iterator, 16> ToInline;
  const auto ItEnd = FieldsMap_.end();
  for (auto It = FieldsMap_.begin(); It != ItEnd; ++It) {
    auto const& CF = *It->second;
    if (CF.anonymous()) {
      if (isa<CompositeType>(CF.getType())) {
        ToInline.emplace_back(It);
      }
    }
  }

  InlinedFields_.reserve(ToInline.size());
  for (auto const& It: ToInline) {
    CompositeField const& CF = *It->second;
    FieldsMap_.erase(It);
    // We need to inline the type of this CF. We adjust the offset of the new
    // fields thanks to the one of CF.
    const unsigned CFOff = CF.getOffset();
    for (auto const& ToInlineF: cast<CompositeType>(CF.getType())->getFields()) {
      const char* Name = ToInlineF.getName();
      InlinedFields_.emplace_back(CompositeField{Name, ToInlineF.getType(), CFOff+ToInlineF.getOffset()});
    }
  }

  for (auto const& CF: InlinedFields_) {
    FieldsMap_[CF.getName()] = &CF;
  }
}

namespace details {

void CUImpl::inlineCompositesAnonymousMembersImpl(std::unordered_set<CompositeType*>& Visited, CompositeType* CTy)
{
  if (!Visited.insert(CTy).second) {
    return;
  }

  SmallVector<CompositeType*, 16> CTys;
  for (auto& CF: CTy->getFields()) {
    if (CF.anonymous()) {
      // TODO: what to do when we have an enum?
      if (auto* CFTy = dyn_cast<CompositeType>(CF.getType())) {
        CTys.push_back(const_cast<CompositeType*>(CFTy));
      }
    }
  }

  for (auto* CFTy: CTys) {
    inlineCompositesAnonymousMembersImpl(Visited, CFTy);
  }

  if (CTys.size() > 0) {
    CTy->inlineAnonymousMembers();
  }
}

void CUImpl::inlineCompositesAnonymousMembers()
{
  std::unordered_set<CompositeType*> Visited;
  for (auto const& It: CompositeTys_) {
    if (auto* CTy = dyn_cast<CompositeType>(It.getValue().get())) {
      inlineCompositesAnonymousMembersImpl(Visited, CTy);
    }
  }
#if 0
  SetVector<CompositeType*> OrderedCTys;

  {
    SmallVector<CompositeType*, 8> WorkList;
    for (auto const& It: CompositeTys_) {
      WorkList.push_back(It->second.get());
    }
    std::unordered_set<CompositeType*> Visited;

    while (!WorkList.empty()) {
      CompositeType* Cur = WorkList.back();
      WorkList.pop_back();
      if (!Cur.insert(Visisted).second) {
        continue;
      }
      bool Found = false;
      for (auto& CF: Cur->getFields()) {
        if (CF.getName().empty()) {
          Found = true;
          WorkList.push_back(CF.getType());
        }
      }
      if (Found) {
        OrderedCTys.insert(Cur);
      }
    }
  }

  // Let's walk through OrderedCTys backwards, and inline anonymous members
  const auto ItEnd = OrderedCTys.end();
  for (auto It = OrderedCTys.rbegin(); It != ItEnd; ++It) {
    (*It)->inlineAnonymousMembers();
  }
#endif
}

} // details
} // dffi
