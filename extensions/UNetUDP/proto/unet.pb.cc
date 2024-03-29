// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: unet.proto

#include "unet.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace uniset3
{
    namespace unet
    {
        PROTOBUF_CONSTEXPR UNetData::UNetData(
            ::_pbi::ConstantInitialized): _impl_
        {
            /*decltype(_impl_.did_)*/{}
            , /*decltype(_impl_._did_cached_byte_size_)*/{0}
            , /*decltype(_impl_.dvalue_)*/{}
            , /*decltype(_impl_.aid_)*/{}
            , /*decltype(_impl_._aid_cached_byte_size_)*/{0}
            , /*decltype(_impl_.avalue_)*/{}
            , /*decltype(_impl_._avalue_cached_byte_size_)*/{0}
            , /*decltype(_impl_._cached_size_)*/{}} {}
        struct UNetDataDefaultTypeInternal
        {
            PROTOBUF_CONSTEXPR UNetDataDefaultTypeInternal()
                : _instance(::_pbi::ConstantInitialized{}) {}
            ~UNetDataDefaultTypeInternal() {}
            union
                {
                    UNetData _instance;
                };
        };
        PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 UNetDataDefaultTypeInternal _UNetData_default_instance_;
        PROTOBUF_CONSTEXPR UNetPacket::UNetPacket(
            ::_pbi::ConstantInitialized): _impl_
        {
            /*decltype(_impl_._has_bits_)*/{}
            , /*decltype(_impl_._cached_size_)*/{}
            , /*decltype(_impl_.data_)*/nullptr
            , /*decltype(_impl_.num_)*/uint64_t{0u}
            , /*decltype(_impl_.nodeid_)*/uint64_t{0u}
            , /*decltype(_impl_.procid_)*/uint64_t{0u}
            , /*decltype(_impl_.magic_)*/0u} {}
        struct UNetPacketDefaultTypeInternal
        {
            PROTOBUF_CONSTEXPR UNetPacketDefaultTypeInternal()
                : _instance(::_pbi::ConstantInitialized{}) {}
            ~UNetPacketDefaultTypeInternal() {}
            union
                {
                    UNetPacket _instance;
                };
        };
        PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 UNetPacketDefaultTypeInternal _UNetPacket_default_instance_;
    }  // namespace unet
}  // namespace uniset3
namespace uniset3
{
    namespace unet
    {

        // ===================================================================

        class UNetData::_Internal
        {
            public:
        };

        UNetData::UNetData(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                           bool is_message_owned)
            : ::PROTOBUF_NAMESPACE_ID::MessageLite(arena, is_message_owned)
        {
            SharedCtor(arena, is_message_owned);
            // @@protoc_insertion_point(arena_constructor:uniset3.unet.UNetData)
        }
        UNetData::UNetData(const UNetData& from)
            : ::PROTOBUF_NAMESPACE_ID::MessageLite()
        {
            UNetData* const _this = this;
            (void)_this;
            new (&_impl_) Impl_
            {
                decltype(_impl_.did_)
                {
                    from._impl_.did_
                }
                , /*decltype(_impl_._did_cached_byte_size_)*/{0}
                , decltype(_impl_.dvalue_)
                {
                    from._impl_.dvalue_
                }
                , decltype(_impl_.aid_)
                {
                    from._impl_.aid_
                }
                , /*decltype(_impl_._aid_cached_byte_size_)*/{0}
                , decltype(_impl_.avalue_)
                {
                    from._impl_.avalue_
                }
                , /*decltype(_impl_._avalue_cached_byte_size_)*/{0}
                , /*decltype(_impl_._cached_size_)*/{}};

            _internal_metadata_.MergeFrom<std::string>(from._internal_metadata_);
            // @@protoc_insertion_point(copy_constructor:uniset3.unet.UNetData)
        }

        inline void UNetData::SharedCtor(
            ::_pb::Arena* arena, bool is_message_owned)
        {
            (void)arena;
            (void)is_message_owned;
            new (&_impl_) Impl_
            {
                decltype(_impl_.did_)
                {
                    arena
                }
                , /*decltype(_impl_._did_cached_byte_size_)*/{0}
                , decltype(_impl_.dvalue_)
                {
                    arena
                }
                , decltype(_impl_.aid_)
                {
                    arena
                }
                , /*decltype(_impl_._aid_cached_byte_size_)*/{0}
                , decltype(_impl_.avalue_)
                {
                    arena
                }
                , /*decltype(_impl_._avalue_cached_byte_size_)*/{0}
                , /*decltype(_impl_._cached_size_)*/{}
            };
        }

        UNetData::~UNetData()
        {
            // @@protoc_insertion_point(destructor:uniset3.unet.UNetData)
            if (auto* arena = _internal_metadata_.DeleteReturnArena<std::string>())
            {
                (void)arena;
                return;
            }

            SharedDtor();
        }

        inline void UNetData::SharedDtor()
        {
            GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
            _impl_.did_.~RepeatedField();
            _impl_.dvalue_.~RepeatedField();
            _impl_.aid_.~RepeatedField();
            _impl_.avalue_.~RepeatedField();
        }

        void UNetData::SetCachedSize(int size) const
        {
            _impl_._cached_size_.Set(size);
        }

        void UNetData::Clear()
        {
            // @@protoc_insertion_point(message_clear_start:uniset3.unet.UNetData)
            uint32_t cached_has_bits = 0;
            // Prevent compiler warnings about cached_has_bits being unused
            (void) cached_has_bits;

            _impl_.did_.Clear();
            _impl_.dvalue_.Clear();
            _impl_.aid_.Clear();
            _impl_.avalue_.Clear();
            _internal_metadata_.Clear<std::string>();
        }

        const char* UNetData::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx)
        {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure

            while (!ctx->Done(&ptr))
            {
                uint32_t tag;
                ptr = ::_pbi::ReadTag(ptr, &tag);

                switch (tag >> 3)
                {
                    // repeated uint64 dID = 1 [packed = true];
                    case 1:
                        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10))
                        {
                            ptr = ::PROTOBUF_NAMESPACE_ID::internal::PackedUInt64Parser(_internal_mutable_did(), ptr, ctx);
                            CHK_(ptr);
                        }
                        else if (static_cast<uint8_t>(tag) == 8)
                        {
                            _internal_add_did(::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr));
                            CHK_(ptr);
                        }
                        else
                            goto handle_unusual;

                        continue;

                    // repeated bool dValue = 2 [packed = true];
                    case 2:
                        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 18))
                        {
                            ptr = ::PROTOBUF_NAMESPACE_ID::internal::PackedBoolParser(_internal_mutable_dvalue(), ptr, ctx);
                            CHK_(ptr);
                        }
                        else if (static_cast<uint8_t>(tag) == 16)
                        {
                            _internal_add_dvalue(::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr));
                            CHK_(ptr);
                        }
                        else
                            goto handle_unusual;

                        continue;

                    // repeated uint64 aID = 3 [packed = true];
                    case 3:
                        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 26))
                        {
                            ptr = ::PROTOBUF_NAMESPACE_ID::internal::PackedUInt64Parser(_internal_mutable_aid(), ptr, ctx);
                            CHK_(ptr);
                        }
                        else if (static_cast<uint8_t>(tag) == 24)
                        {
                            _internal_add_aid(::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr));
                            CHK_(ptr);
                        }
                        else
                            goto handle_unusual;

                        continue;

                    // repeated uint64 aValue = 4 [packed = true];
                    case 4:
                        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 34))
                        {
                            ptr = ::PROTOBUF_NAMESPACE_ID::internal::PackedUInt64Parser(_internal_mutable_avalue(), ptr, ctx);
                            CHK_(ptr);
                        }
                        else if (static_cast<uint8_t>(tag) == 32)
                        {
                            _internal_add_avalue(::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr));
                            CHK_(ptr);
                        }
                        else
                            goto handle_unusual;

                        continue;

                    default:
                        goto handle_unusual;
                }  // switch

            handle_unusual:

                if ((tag == 0) || ((tag & 7) == 4))
                {
                    CHK_(ptr);
                    ctx->SetLastTag(tag);
                    goto message_done;
                }

                ptr = UnknownFieldParse(
                          tag,
                          _internal_metadata_.mutable_unknown_fields<std::string>(),
                          ptr, ctx);
                CHK_(ptr != nullptr);
            }  // while

        message_done:
            return ptr;
        failure:
            ptr = nullptr;
            goto message_done;
#undef CHK_
        }

        uint8_t* UNetData::_InternalSerialize(
            uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const
        {
            // @@protoc_insertion_point(serialize_to_array_start:uniset3.unet.UNetData)
            uint32_t cached_has_bits = 0;
            (void) cached_has_bits;

            // repeated uint64 dID = 1 [packed = true];
            {
                int byte_size = _impl_._did_cached_byte_size_.load(std::memory_order_relaxed);

                if (byte_size > 0)
                {
                    target = stream->WriteUInt64Packed(
                                 1, _internal_did(), byte_size, target);
                }
            }

            // repeated bool dValue = 2 [packed = true];
            if (this->_internal_dvalue_size() > 0)
            {
                target = stream->WriteFixedPacked(2, _internal_dvalue(), target);
            }

            // repeated uint64 aID = 3 [packed = true];
            {
                int byte_size = _impl_._aid_cached_byte_size_.load(std::memory_order_relaxed);

                if (byte_size > 0)
                {
                    target = stream->WriteUInt64Packed(
                                 3, _internal_aid(), byte_size, target);
                }
            }

            // repeated uint64 aValue = 4 [packed = true];
            {
                int byte_size = _impl_._avalue_cached_byte_size_.load(std::memory_order_relaxed);

                if (byte_size > 0)
                {
                    target = stream->WriteUInt64Packed(
                                 4, _internal_avalue(), byte_size, target);
                }
            }

            if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields()))
            {
                target = stream->WriteRaw(_internal_metadata_.unknown_fields<std::string>(::PROTOBUF_NAMESPACE_ID::internal::GetEmptyString).data(),
                                          static_cast<int>(_internal_metadata_.unknown_fields<std::string>(::PROTOBUF_NAMESPACE_ID::internal::GetEmptyString).size()), target);
            }

            // @@protoc_insertion_point(serialize_to_array_end:uniset3.unet.UNetData)
            return target;
        }

        size_t UNetData::ByteSizeLong() const
        {
            // @@protoc_insertion_point(message_byte_size_start:uniset3.unet.UNetData)
            size_t total_size = 0;

            uint32_t cached_has_bits = 0;
            // Prevent compiler warnings about cached_has_bits being unused
            (void) cached_has_bits;

            // repeated uint64 dID = 1 [packed = true];
            {
                size_t data_size = ::_pbi::WireFormatLite::
                                   UInt64Size(this->_impl_.did_);

                if (data_size > 0)
                {
                    total_size += 1 +
                                  ::_pbi::WireFormatLite::Int32Size(static_cast<int32_t>(data_size));
                }

                int cached_size = ::_pbi::ToCachedSize(data_size);
                _impl_._did_cached_byte_size_.store(cached_size,
                                                    std::memory_order_relaxed);
                total_size += data_size;
            }

            // repeated bool dValue = 2 [packed = true];
            {
                unsigned int count = static_cast<unsigned int>(this->_internal_dvalue_size());
                size_t data_size = 1UL * count;

                if (data_size > 0)
                {
                    total_size += 1 +
                                  ::_pbi::WireFormatLite::Int32Size(static_cast<int32_t>(data_size));
                }

                total_size += data_size;
            }

            // repeated uint64 aID = 3 [packed = true];
            {
                size_t data_size = ::_pbi::WireFormatLite::
                                   UInt64Size(this->_impl_.aid_);

                if (data_size > 0)
                {
                    total_size += 1 +
                                  ::_pbi::WireFormatLite::Int32Size(static_cast<int32_t>(data_size));
                }

                int cached_size = ::_pbi::ToCachedSize(data_size);
                _impl_._aid_cached_byte_size_.store(cached_size,
                                                    std::memory_order_relaxed);
                total_size += data_size;
            }

            // repeated uint64 aValue = 4 [packed = true];
            {
                size_t data_size = ::_pbi::WireFormatLite::
                                   UInt64Size(this->_impl_.avalue_);

                if (data_size > 0)
                {
                    total_size += 1 +
                                  ::_pbi::WireFormatLite::Int32Size(static_cast<int32_t>(data_size));
                }

                int cached_size = ::_pbi::ToCachedSize(data_size);
                _impl_._avalue_cached_byte_size_.store(cached_size,
                                                       std::memory_order_relaxed);
                total_size += data_size;
            }

            if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields()))
            {
                total_size += _internal_metadata_.unknown_fields<std::string>(::PROTOBUF_NAMESPACE_ID::internal::GetEmptyString).size();
            }

            int cached_size = ::_pbi::ToCachedSize(total_size);
            SetCachedSize(cached_size);
            return total_size;
        }

        void UNetData::CheckTypeAndMergeFrom(
            const ::PROTOBUF_NAMESPACE_ID::MessageLite& from)
        {
            MergeFrom(*::_pbi::DownCast<const UNetData*>(
                          &from));
        }

        void UNetData::MergeFrom(const UNetData& from)
        {
            UNetData* const _this = this;
            // @@protoc_insertion_point(class_specific_merge_from_start:uniset3.unet.UNetData)
            GOOGLE_DCHECK_NE(&from, _this);
            uint32_t cached_has_bits = 0;
            (void) cached_has_bits;

            _this->_impl_.did_.MergeFrom(from._impl_.did_);
            _this->_impl_.dvalue_.MergeFrom(from._impl_.dvalue_);
            _this->_impl_.aid_.MergeFrom(from._impl_.aid_);
            _this->_impl_.avalue_.MergeFrom(from._impl_.avalue_);
            _this->_internal_metadata_.MergeFrom<std::string>(from._internal_metadata_);
        }

        void UNetData::CopyFrom(const UNetData& from)
        {
            // @@protoc_insertion_point(class_specific_copy_from_start:uniset3.unet.UNetData)
            if (&from == this) return;

            Clear();
            MergeFrom(from);
        }

        bool UNetData::IsInitialized() const
        {
            return true;
        }

        void UNetData::InternalSwap(UNetData* other)
        {
            using std::swap;
            _internal_metadata_.InternalSwap(&other->_internal_metadata_);
            _impl_.did_.InternalSwap(&other->_impl_.did_);
            _impl_.dvalue_.InternalSwap(&other->_impl_.dvalue_);
            _impl_.aid_.InternalSwap(&other->_impl_.aid_);
            _impl_.avalue_.InternalSwap(&other->_impl_.avalue_);
        }

        std::string UNetData::GetTypeName() const
        {
            return "uniset3.unet.UNetData";
        }


        // ===================================================================

        class UNetPacket::_Internal
        {
            public:
                using HasBits = decltype(std::declval<UNetPacket>()._impl_._has_bits_);
                static void set_has_magic(HasBits* has_bits)
                {
                    (*has_bits)[0] |= 16u;
                }
                static void set_has_num(HasBits* has_bits)
                {
                    (*has_bits)[0] |= 2u;
                }
                static void set_has_nodeid(HasBits* has_bits)
                {
                    (*has_bits)[0] |= 4u;
                }
                static void set_has_procid(HasBits* has_bits)
                {
                    (*has_bits)[0] |= 8u;
                }
                static const ::uniset3::unet::UNetData& data(const UNetPacket* msg);
                static void set_has_data(HasBits* has_bits)
                {
                    (*has_bits)[0] |= 1u;
                }
                static bool MissingRequiredFields(const HasBits& has_bits)
                {
                    return ((has_bits[0] & 0x00000012) ^ 0x00000012) != 0;
                }
        };

        const ::uniset3::unet::UNetData&
        UNetPacket::_Internal::data(const UNetPacket* msg)
        {
            return *msg->_impl_.data_;
        }
        UNetPacket::UNetPacket(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                               bool is_message_owned)
            : ::PROTOBUF_NAMESPACE_ID::MessageLite(arena, is_message_owned)
        {
            SharedCtor(arena, is_message_owned);
            // @@protoc_insertion_point(arena_constructor:uniset3.unet.UNetPacket)
        }
        UNetPacket::UNetPacket(const UNetPacket& from)
            : ::PROTOBUF_NAMESPACE_ID::MessageLite()
        {
            UNetPacket* const _this = this;
            (void)_this;
            new (&_impl_) Impl_
            {
                decltype(_impl_._has_bits_)
                {
                    from._impl_._has_bits_
                }
                , /*decltype(_impl_._cached_size_)*/{}
                , decltype(_impl_.data_)
                {
                    nullptr
                }
                , decltype(_impl_.num_) {}
                , decltype(_impl_.nodeid_) {}
                , decltype(_impl_.procid_) {}
                , decltype(_impl_.magic_) {}};

            _internal_metadata_.MergeFrom<std::string>(from._internal_metadata_);

            if (from._internal_has_data())
            {
                _this->_impl_.data_ = new ::uniset3::unet::UNetData(*from._impl_.data_);
            }

            ::memcpy(&_impl_.num_, &from._impl_.num_,
                     static_cast<size_t>(reinterpret_cast<char*>(&_impl_.magic_) -
                                         reinterpret_cast<char*>(&_impl_.num_)) + sizeof(_impl_.magic_));
            // @@protoc_insertion_point(copy_constructor:uniset3.unet.UNetPacket)
        }

        inline void UNetPacket::SharedCtor(
            ::_pb::Arena* arena, bool is_message_owned)
        {
            (void)arena;
            (void)is_message_owned;
            new (&_impl_) Impl_
            {
                decltype(_impl_._has_bits_) {}
                , /*decltype(_impl_._cached_size_)*/{}
                , decltype(_impl_.data_)
                {
                    nullptr
                }
                , decltype(_impl_.num_)
                {
                    uint64_t{0u}
                }
                , decltype(_impl_.nodeid_)
                {
                    uint64_t{0u}
                }
                , decltype(_impl_.procid_)
                {
                    uint64_t{0u}
                }
                , decltype(_impl_.magic_)
                {
                    0u
                }
            };
        }

        UNetPacket::~UNetPacket()
        {
            // @@protoc_insertion_point(destructor:uniset3.unet.UNetPacket)
            if (auto* arena = _internal_metadata_.DeleteReturnArena<std::string>())
            {
                (void)arena;
                return;
            }

            SharedDtor();
        }

        inline void UNetPacket::SharedDtor()
        {
            GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);

            if (this != internal_default_instance()) delete _impl_.data_;
        }

        void UNetPacket::SetCachedSize(int size) const
        {
            _impl_._cached_size_.Set(size);
        }

        void UNetPacket::Clear()
        {
            // @@protoc_insertion_point(message_clear_start:uniset3.unet.UNetPacket)
            uint32_t cached_has_bits = 0;
            // Prevent compiler warnings about cached_has_bits being unused
            (void) cached_has_bits;

            cached_has_bits = _impl_._has_bits_[0];

            if (cached_has_bits & 0x00000001u)
            {
                GOOGLE_DCHECK(_impl_.data_ != nullptr);
                _impl_.data_->Clear();
            }

            if (cached_has_bits & 0x0000001eu)
            {
                ::memset(&_impl_.num_, 0, static_cast<size_t>(
                             reinterpret_cast<char*>(&_impl_.magic_) -
                             reinterpret_cast<char*>(&_impl_.num_)) + sizeof(_impl_.magic_));
            }

            _impl_._has_bits_.Clear();
            _internal_metadata_.Clear<std::string>();
        }

        const char* UNetPacket::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx)
        {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
            _Internal::HasBits has_bits {};

            while (!ctx->Done(&ptr))
            {
                uint32_t tag;
                ptr = ::_pbi::ReadTag(ptr, &tag);

                switch (tag >> 3)
                {
                    // required uint32 magic = 1;
                    case 1:
                        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 8))
                        {
                            _Internal::set_has_magic(&has_bits);
                            _impl_.magic_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
                            CHK_(ptr);
                        }
                        else
                            goto handle_unusual;

                        continue;

                    // required uint64 num = 2;
                    case 2:
                        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 16))
                        {
                            _Internal::set_has_num(&has_bits);
                            _impl_.num_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
                            CHK_(ptr);
                        }
                        else
                            goto handle_unusual;

                        continue;

                    // optional uint64 nodeID = 3;
                    case 3:
                        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 24))
                        {
                            _Internal::set_has_nodeid(&has_bits);
                            _impl_.nodeid_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
                            CHK_(ptr);
                        }
                        else
                            goto handle_unusual;

                        continue;

                    // optional uint64 procID = 4;
                    case 4:
                        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 32))
                        {
                            _Internal::set_has_procid(&has_bits);
                            _impl_.procid_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
                            CHK_(ptr);
                        }
                        else
                            goto handle_unusual;

                        continue;

                    // optional .uniset3.unet.UNetData data = 5;
                    case 5:
                        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 42))
                        {
                            ptr = ctx->ParseMessage(_internal_mutable_data(), ptr);
                            CHK_(ptr);
                        }
                        else
                            goto handle_unusual;

                        continue;

                    default:
                        goto handle_unusual;
                }  // switch

            handle_unusual:

                if ((tag == 0) || ((tag & 7) == 4))
                {
                    CHK_(ptr);
                    ctx->SetLastTag(tag);
                    goto message_done;
                }

                ptr = UnknownFieldParse(
                          tag,
                          _internal_metadata_.mutable_unknown_fields<std::string>(),
                          ptr, ctx);
                CHK_(ptr != nullptr);
            }  // while

        message_done:
            _impl_._has_bits_.Or(has_bits);
            return ptr;
        failure:
            ptr = nullptr;
            goto message_done;
#undef CHK_
        }

        uint8_t* UNetPacket::_InternalSerialize(
            uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const
        {
            // @@protoc_insertion_point(serialize_to_array_start:uniset3.unet.UNetPacket)
            uint32_t cached_has_bits = 0;
            (void) cached_has_bits;

            cached_has_bits = _impl_._has_bits_[0];

            // required uint32 magic = 1;
            if (cached_has_bits & 0x00000010u)
            {
                target = stream->EnsureSpace(target);
                target = ::_pbi::WireFormatLite::WriteUInt32ToArray(1, this->_internal_magic(), target);
            }

            // required uint64 num = 2;
            if (cached_has_bits & 0x00000002u)
            {
                target = stream->EnsureSpace(target);
                target = ::_pbi::WireFormatLite::WriteUInt64ToArray(2, this->_internal_num(), target);
            }

            // optional uint64 nodeID = 3;
            if (cached_has_bits & 0x00000004u)
            {
                target = stream->EnsureSpace(target);
                target = ::_pbi::WireFormatLite::WriteUInt64ToArray(3, this->_internal_nodeid(), target);
            }

            // optional uint64 procID = 4;
            if (cached_has_bits & 0x00000008u)
            {
                target = stream->EnsureSpace(target);
                target = ::_pbi::WireFormatLite::WriteUInt64ToArray(4, this->_internal_procid(), target);
            }

            // optional .uniset3.unet.UNetData data = 5;
            if (cached_has_bits & 0x00000001u)
            {
                target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
                         InternalWriteMessage(5, _Internal::data(this),
                                              _Internal::data(this).GetCachedSize(), target, stream);
            }

            if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields()))
            {
                target = stream->WriteRaw(_internal_metadata_.unknown_fields<std::string>(::PROTOBUF_NAMESPACE_ID::internal::GetEmptyString).data(),
                                          static_cast<int>(_internal_metadata_.unknown_fields<std::string>(::PROTOBUF_NAMESPACE_ID::internal::GetEmptyString).size()), target);
            }

            // @@protoc_insertion_point(serialize_to_array_end:uniset3.unet.UNetPacket)
            return target;
        }

        size_t UNetPacket::RequiredFieldsByteSizeFallback() const
        {
            // @@protoc_insertion_point(required_fields_byte_size_fallback_start:uniset3.unet.UNetPacket)
            size_t total_size = 0;

            if (_internal_has_num())
            {
                // required uint64 num = 2;
                total_size += ::_pbi::WireFormatLite::UInt64SizePlusOne(this->_internal_num());
            }

            if (_internal_has_magic())
            {
                // required uint32 magic = 1;
                total_size += ::_pbi::WireFormatLite::UInt32SizePlusOne(this->_internal_magic());
            }

            return total_size;
        }
        size_t UNetPacket::ByteSizeLong() const
        {
            // @@protoc_insertion_point(message_byte_size_start:uniset3.unet.UNetPacket)
            size_t total_size = 0;

            if (((_impl_._has_bits_[0] & 0x00000012) ^ 0x00000012) == 0)    // All required fields are present.
            {
                // required uint64 num = 2;
                total_size += ::_pbi::WireFormatLite::UInt64SizePlusOne(this->_internal_num());

                // required uint32 magic = 1;
                total_size += ::_pbi::WireFormatLite::UInt32SizePlusOne(this->_internal_magic());

            }
            else
            {
                total_size += RequiredFieldsByteSizeFallback();
            }

            uint32_t cached_has_bits = 0;
            // Prevent compiler warnings about cached_has_bits being unused
            (void) cached_has_bits;

            // optional .uniset3.unet.UNetData data = 5;
            cached_has_bits = _impl_._has_bits_[0];

            if (cached_has_bits & 0x00000001u)
            {
                total_size += 1 +
                              ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(
                                  *_impl_.data_);
            }

            if (cached_has_bits & 0x0000000cu)
            {
                // optional uint64 nodeID = 3;
                if (cached_has_bits & 0x00000004u)
                {
                    total_size += ::_pbi::WireFormatLite::UInt64SizePlusOne(this->_internal_nodeid());
                }

                // optional uint64 procID = 4;
                if (cached_has_bits & 0x00000008u)
                {
                    total_size += ::_pbi::WireFormatLite::UInt64SizePlusOne(this->_internal_procid());
                }

            }

            if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields()))
            {
                total_size += _internal_metadata_.unknown_fields<std::string>(::PROTOBUF_NAMESPACE_ID::internal::GetEmptyString).size();
            }

            int cached_size = ::_pbi::ToCachedSize(total_size);
            SetCachedSize(cached_size);
            return total_size;
        }

        void UNetPacket::CheckTypeAndMergeFrom(
            const ::PROTOBUF_NAMESPACE_ID::MessageLite& from)
        {
            MergeFrom(*::_pbi::DownCast<const UNetPacket*>(
                          &from));
        }

        void UNetPacket::MergeFrom(const UNetPacket& from)
        {
            UNetPacket* const _this = this;
            // @@protoc_insertion_point(class_specific_merge_from_start:uniset3.unet.UNetPacket)
            GOOGLE_DCHECK_NE(&from, _this);
            uint32_t cached_has_bits = 0;
            (void) cached_has_bits;

            cached_has_bits = from._impl_._has_bits_[0];

            if (cached_has_bits & 0x0000001fu)
            {
                if (cached_has_bits & 0x00000001u)
                {
                    _this->_internal_mutable_data()->::uniset3::unet::UNetData::MergeFrom(
                        from._internal_data());
                }

                if (cached_has_bits & 0x00000002u)
                {
                    _this->_impl_.num_ = from._impl_.num_;
                }

                if (cached_has_bits & 0x00000004u)
                {
                    _this->_impl_.nodeid_ = from._impl_.nodeid_;
                }

                if (cached_has_bits & 0x00000008u)
                {
                    _this->_impl_.procid_ = from._impl_.procid_;
                }

                if (cached_has_bits & 0x00000010u)
                {
                    _this->_impl_.magic_ = from._impl_.magic_;
                }

                _this->_impl_._has_bits_[0] |= cached_has_bits;
            }

            _this->_internal_metadata_.MergeFrom<std::string>(from._internal_metadata_);
        }

        void UNetPacket::CopyFrom(const UNetPacket& from)
        {
            // @@protoc_insertion_point(class_specific_copy_from_start:uniset3.unet.UNetPacket)
            if (&from == this) return;

            Clear();
            MergeFrom(from);
        }

        bool UNetPacket::IsInitialized() const
        {
            if (_Internal::MissingRequiredFields(_impl_._has_bits_)) return false;

            return true;
        }

        void UNetPacket::InternalSwap(UNetPacket* other)
        {
            using std::swap;
            _internal_metadata_.InternalSwap(&other->_internal_metadata_);
            swap(_impl_._has_bits_[0], other->_impl_._has_bits_[0]);
            ::PROTOBUF_NAMESPACE_ID::internal::memswap <
            PROTOBUF_FIELD_OFFSET(UNetPacket, _impl_.magic_)
            + sizeof(UNetPacket::_impl_.magic_)
            - PROTOBUF_FIELD_OFFSET(UNetPacket, _impl_.data_) > (
                reinterpret_cast<char*>(&_impl_.data_),
                reinterpret_cast<char*>(&other->_impl_.data_));
        }

        std::string UNetPacket::GetTypeName() const
        {
            return "uniset3.unet.UNetPacket";
        }


        // @@protoc_insertion_point(namespace_scope)
    }  // namespace unet
}  // namespace uniset3
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::uniset3::unet::UNetData*
Arena::CreateMaybeMessage< ::uniset3::unet::UNetData >(Arena* arena)
{
    return Arena::CreateMessageInternal< ::uniset3::unet::UNetData >(arena);
}
template<> PROTOBUF_NOINLINE ::uniset3::unet::UNetPacket*
Arena::CreateMaybeMessage< ::uniset3::unet::UNetPacket >(Arena* arena)
{
    return Arena::CreateMessageInternal< ::uniset3::unet::UNetPacket >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
