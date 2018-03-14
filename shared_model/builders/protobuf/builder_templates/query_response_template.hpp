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

#ifndef IROHA_PROTO_QUERY_RESPONSE_BUILDER_TEMPLATE_HPP
#define IROHA_PROTO_QUERY_RESPONSE_BUILDER_TEMPLATE_HPP

#include "backend/protobuf/query_responses/proto_query_response.hpp"
#include "builders/protobuf/helpers.hpp"
#include "common/visitor.hpp"
#include "interfaces/common_objects/types.hpp"
#include "responses.pb.h"

namespace shared_model {
  namespace proto {

    class ReasonSetter {
     public:
      template <class T>
      static void setReason(iroha::protocol::ErrorResponse &err){};
    };

    template <>
    void ReasonSetter::setReason<interface::StatelessFailedErrorResponse>(
        iroha::protocol::ErrorResponse &err) {
      err.set_reason(iroha::protocol::ErrorResponse_Reason_STATELESS_INVALID);
    }

    template <>
    void ReasonSetter::setReason<interface::StatefulFailedErrorResponse>(
        iroha::protocol::ErrorResponse &err) {
      err.set_reason(iroha::protocol::ErrorResponse_Reason_STATEFUL_INVALID);
    }

    template <>
    void ReasonSetter::setReason<interface::NoAccountErrorResponse>(
        iroha::protocol::ErrorResponse &err) {
      err.set_reason(iroha::protocol::ErrorResponse_Reason_NO_ACCOUNT);
    }

    template <>
    void ReasonSetter::setReason<interface::NoAccountAssetsErrorResponse>(
        iroha::protocol::ErrorResponse &err) {
      err.set_reason(iroha::protocol::ErrorResponse_Reason_NO_ACCOUNT_ASSETS);
    }

    template <>
    void ReasonSetter::setReason<interface::NoAccountDetailErrorResponse>(
        iroha::protocol::ErrorResponse &err) {
      err.set_reason(iroha::protocol::ErrorResponse_Reason_NO_ACCOUNT_DETAIL);
    }

    template <>
    void ReasonSetter::setReason<interface::NoSignatoriesErrorResponse>(
        iroha::protocol::ErrorResponse &err) {
      err.set_reason(iroha::protocol::ErrorResponse_Reason_NO_SIGNATORIES);
    }

    template <>
    void ReasonSetter::setReason<interface::NotSupportedErrorResponse>(
        iroha::protocol::ErrorResponse &err) {
      err.set_reason(iroha::protocol::ErrorResponse_Reason_NOT_SUPPORTED);
    }

    template <>
    void ReasonSetter::setReason<interface::NoAssetErrorResponse>(
        iroha::protocol::ErrorResponse &err) {
      err.set_reason(iroha::protocol::ErrorResponse_Reason_NO_ASSET);
    }

    template <>
    void ReasonSetter::setReason<interface::NoRolesErrorResponse>(
        iroha::protocol::ErrorResponse &err) {
      err.set_reason(iroha::protocol::ErrorResponse_Reason_NO_ROLES);
    }

    /**
     * Template query response builder for creating new types of query response
     * builders by means of replacing template parameters
     * @tparam S -- field counter for checking that all required fields are
     * set
     * @tparam BT -- build type of built object returned by build method
     */
    template <int S = 0, typename BT = QueryResponse>
    class TemplateQueryResponseBuilder {
     private:
      template <int, typename>
      friend class TemplateQueryResponseBuilder;

      enum RequiredFields { QueryResponseField, QueryHash, TOTAL };

      template <int s>
      using NextBuilder = TemplateQueryResponseBuilder<S | (1 << s), BT>;

      using ProtoQueryResponse = iroha::protocol::QueryResponse;

      template <class T>
      using w = shared_model::detail::PolymorphicWrapper<T>;

      template <int Sp>
      TemplateQueryResponseBuilder(
          const TemplateQueryResponseBuilder<Sp, BT> &o)
          : query_response_(o.query_response_) {}

      /**
       * Make transformation on copied content
       * @tparam Transformation - callable type for changing the copy
       * @param t - transform function for proto object
       * @return new builder with updated state
       */
      template <int Fields, typename Transformation>
      auto transform(Transformation t) const {
        NextBuilder<Fields> copy = *this;
        t(copy.query_response_);
        return copy;
      }

      /**
       * Make query field transformation on copied object
       * @tparam Transformation - callable type for changing query
       * @param t - transform function for proto query
       * @return new builder with set query
       */
      template <typename Transformation>
      auto queryResponseField(Transformation t) const {
        NextBuilder<QueryResponseField> copy = *this;
        t(copy.query_response_);
        return copy;
      }

     public:
      TemplateQueryResponseBuilder() = default;

      auto accountAssetResponse(
          const interface::types::AssetIdType &asset_id,
          const interface::types::AccountIdType &account_id,
          const std::string &amount) const {
        return queryResponseField([&](auto &proto_query_response) {
          iroha::protocol::AccountAssetResponse *query_response =
              proto_query_response.mutable_account_assets_response();

          query_response->mutable_account_asset()->set_account_id(account_id);
          query_response->mutable_account_asset()->set_asset_id(asset_id);
          initializeProtobufAmount(
              query_response->mutable_account_asset()->mutable_balance(),
              amount);
        });
      }

      auto accountDetailResponse(
          const interface::types::DetailType &account_detail) const {
        return queryResponseField([&](auto &proto_query_response) {
          iroha::protocol::AccountDetailResponse *query_response =
              proto_query_response.mutable_account_detail_response();
          query_response->set_detail(account_detail);
        });
      }

      template <class T>
      auto errorQueryResponse() const {
        return queryResponseField([&](auto &proto_query_response) {
          iroha::protocol::ErrorResponse *query_response =
              proto_query_response.mutable_error_response();
          ReasonSetter::setReason<T>(*query_response);
        });
      }

      auto signatoriesResponse(
          const std::vector<interface::types::BlobType> &signatories) const {
        return queryResponseField([&](auto &proto_query_response) {
          iroha::protocol::SignatoriesResponse *query_response =
              proto_query_response->mutable_signatories_response();
          for (const auto &key : signatories) {
            const auto &blob = key.blob();
            query_response->add_keys(blob.data(), blob.size());
          }
        });
      }

      auto transactionsResponse(
          const std::vector<proto::Transaction> &transactions) const {
        return queryResponseField([&](auto &proto_query_response) {
          iroha::protocol::TransactionsResponse *query_response =
              proto_query_response->mutable_transactions_response();
          for (const auto &tx : transactions) {
            query_response->add_transactions()->CopyFrom(tx.getTransport());
          }
        });
      }

      auto assetResponse(const std::string &asset_id,
                         const std::string &domain_id,
                         const uint32_t precision) const {
        return queryResponseField([&](auto &proto_query_response) {
          iroha::protocol::AssetResponse *query_response =
              proto_query_response->mutable_asset_response();
          auto asset = query_response->mutable_asset();
          asset->set_asset_id(asset_id);
          asset->set_domain_id(domain_id);
          asset->set_precision(precision);
        });
      }

      auto rolesResponse(const interface::RolesResponse &roles) const {
        return queryResponseField([&](auto &proto_query_response) {
          iroha::protocol::RolesResponse *query_response =
              proto_query_response->mutable_roles_response();
          for (const auto &role : roles.roles()) {
            query_response->add_roles(role);
          }
        });
      }

      auto rolePermissionsResponse(
          const interface::RolePermissionsResponse &role_permissions) const {
        return queryResponseField([&](auto &proto_query_response) {
          iroha::protocol::RolePermissionsResponse *query_response =
              proto_query_response->mutable_role_permissions_response();
          for (const auto &perm : role_permissions.rolePermissions()) {
            query_response->add_permissions(perm);
          }
        });
      }

      auto queryHash(const interface::types::HashType &query_hash) const {
        return transform<QueryHash>([&](auto &proto_query_response) {
          proto_query_response.set_query_hash(
              crypto::toBinaryString(query_hash));
        });
      }

      QueryResponse build() const {
        static_assert(S == (1 << TOTAL) - 1, "Required fields are not set");
        auto result =
            QueryResponse(iroha::protocol::QueryResponse(query_response_));
        return BT(std::move(result));
      }

      static const int total = RequiredFields::TOTAL;

     private:
      ProtoQueryResponse query_response_;
    };
  }  // namespace proto
}  // namespace shared_model

#endif  // IROHA_PROTO_QUERY_RESPONSE_BUILDER_TEMPLATE_HPP