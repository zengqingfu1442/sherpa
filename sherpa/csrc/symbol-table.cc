/**
 * Copyright      2022  Xiaomi Corporation (authors: Fangjun Kuang)
 *
 * See LICENSE for clarification regarding multiple authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sherpa/csrc/symbol-table.h"

#include <fstream>
#include <sstream>

#include "sherpa/csrc/base64-decode.h"
#include "sherpa/csrc/log.h"

namespace sherpa {

SymbolTable::SymbolTable(const std::string &filename) {
  std::ifstream is(filename);
  std::string sym;
  int32_t id;
  while (is >> sym >> id) {
    if (sym.size() >= 3) {
      // For BPE-based models, we replace ▁ with a space
      // Unicode 9601, hex 0x2581, utf8 0xe29681
      const uint8_t *p = reinterpret_cast<const uint8_t *>(sym.c_str());
      if (p[0] == 0xe2 && p[1] == 0x96 && p[2] == 0x81) {
        sym = sym.replace(0, 3, " ");
      }
    }

    SHERPA_CHECK(!sym.empty());
    SHERPA_CHECK_EQ(sym2id_.count(sym), 0) << "Duplicated symbol: " << sym;
    SHERPA_CHECK_EQ(id2sym_.count(id), 0) << "Duplicated ID: " << id;

    sym2id_.insert({sym, id});
    id2sym_.insert({id, sym});
  }
  SHERPA_CHECK(is.eof());
}

std::string SymbolTable::ToString() const {
  std::ostringstream os;
  char sep = ' ';
  for (const auto &p : sym2id_) {
    os << p.first << sep << p.second << "\n";
  }
  return os.str();
}

const std::string &SymbolTable::operator[](int32_t id) const {
  return id2sym_.at(id);
}

int32_t SymbolTable::operator[](const std::string &sym) const {
  return sym2id_.at(sym);
}

bool SymbolTable::Contains(int32_t id) const { return id2sym_.count(id) != 0; }

bool SymbolTable::Contains(const std::string &sym) const {
  return sym2id_.count(sym) != 0;
}

std::ostream &operator<<(std::ostream &os, const SymbolTable &symbol_table) {
  return os << symbol_table.ToString();
}

void SymbolTable::Replace(int32_t id, const std::string &new_sym,
                          const std::string &old_sym) {
  sym2id_.erase(old_sym);

  id2sym_.at(id) = new_sym;
  sym2id_[new_sym] = id;
}

void SymbolTable::ApplyBase64Decode() {
  sym2id_.clear();
  for (auto &p : id2sym_) {
    p.second = Base64Decode(p.second);
    sym2id_[p.second] = p.first;
  }
}

}  // namespace sherpa
