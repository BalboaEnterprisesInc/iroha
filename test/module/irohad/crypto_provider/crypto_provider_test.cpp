/**
 * Copyright Soramitsu Co., Ltd. 2018 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include "crypto_provider/impl/crypto_provider_impl.hpp"
#include "cryptography/ed25519_sha3_impl/internal/ed25519_impl.hpp"
#include "model/generators/query_generator.hpp"
#include "model/generators/transaction_generator.hpp"
#include "model/sha3_hash.hpp"

namespace iroha {
  class CryptoProviderTest : public ::testing::Test {
   public:
    CryptoProviderTest() : provider(create_keypair()) {}

    CryptoProviderImpl provider;
  };

  TEST_F(CryptoProviderTest, SignAndVerifyTransaction) {
    auto model_tx =
        model::generators::TransactionGenerator().generateTransaction(
            "test", 0, {});

    provider.sign(model_tx);
    ASSERT_TRUE(provider.verify(model_tx));

    // now modify transaction's meta, so verify should fail
    model_tx.creator_account_id = "test1";
    ASSERT_FALSE(provider.verify(model_tx));
  }

  TEST_F(CryptoProviderTest, SignAndVerifyQuery) {
    auto query = model::generators::QueryGenerator().generateGetAccount(
        0, "test", 0, "test");

    provider.sign(*query);
    ASSERT_TRUE(provider.verify(*query));

    // modify account id, verification should fail
    query->account_id = "kappa";
    ASSERT_FALSE(provider.verify(*query));
  }

  TEST_F(CryptoProviderTest, SameQueryHashAfterSign) {
    auto query = model::generators::QueryGenerator().generateGetAccount(
        0, "test", 0, "test");

    auto hash = iroha::hash(*query);
    provider.sign(*query);

    auto hash_signed = iroha::hash(*query);
    ASSERT_EQ(hash_signed, hash);
  }
}  // namespace iroha
