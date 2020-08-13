#define DISABLE_PICOJSON // Make sure JWT compiles with this flag

#include <gtest/gtest.h>
#include "jwt-cpp/jwt.h"

#include <rapidjson/allocators.h>
#include <rapidjson/cursorstreamwrapper.h>
#include <rapidjson/document.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/encodings.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/fwd.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/memorybuffer.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/pointer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/reader.h>
#include <rapidjson/schema.h>
#include <rapidjson/stream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

namespace jwt
{
	template <typename Encoding, typename Allocator = rapidjson::MemoryPoolAllocator<>, typename StackAllocator = rapidjson::CrtAllocator>
	struct rapidjson_traits
	{
		using document_type = rapidjson::GenericDocument<Encoding, Allocator, StackAllocator>;
		using string_type = std::string;
		using number_type = double;
		using integer_type = int64_t;
		using boolean_type = bool;
		struct array_type;
		struct object_type;
		template <bool Const>
		struct GenericMemberIterator
		{
		};

		struct value_type
		{
			value_type() = default;
			value_type(const value_type& other)
			{
				value.CopyFrom(other.value);
			}
			value_type& operator =(const value_type& other)
			{
				value.CopyFrom(other.value);
				return *this;
			}
			value_type(value_type&&) = default;
			value_type& operator =(value_type&&) = default;

			explicit value_type(bool val) : value(val) {}
			explicit value_type(int val) : value(val) {}
			explicit value_type(unsigned val) : value(val) {}
			explicit value_type(int64_t val) : value(val) {}
			explicit value_type(uint64_t val) : value(val) {}
			explicit value_type(double val) : value(val) {}
			explicit value_type(float val) : value(val) {}
			explicit value_type(std::string val) : value(val.c_str(), val.length(), Allocator()) {} // TODO: Fix using same allocator

			explicit value_type(const array_type&)
			{
				value.CopyFrom(other.value);
			}
			explicit value_type(array_type&&) : value(std::move(other.value)) { }
			explicit value_type(const object_type&)
			{
				value.CopyFrom(other.value);
			}
			explicit value_type(object_type&&) : value(std::move(other.value)) { }

			using iterator = GenericMemberIterator<false>;
			using const_iterator = GenericMemberIterator<true>;
		protected:
			explicit value_type(rapidjson::Type type) : value(type) {}

			typename document_type::ValueType value;
		};
		struct array_type : value_type
		{
			array_type() : value_type(rapidjson::kArrayType) {}
			array_type(const array_type&) = default;
			array_type& operator =(const array_type&) = default;
			array_type(array_type&&) = default;
			array_type& operator =(array_type&&) = default;

			template <typename T>
			array_type(T begin, T end)
				: array_type()
			{
				for (const auto& it = begin; it != end; ++it)
					value.PushBack(*it);
			}

			value_type& operator[](size_t i)
			{
				return value[value_type(i)];
			}

			const_iterator begin() const -> decltype(value.MemberBegin())
			{
				return value.MemberBegin();
			}

			const_iterator end() const -> decltype(value.MemberEnd())
			{
				return value.MemberEnd();
			}

			iterator begin() -> decltype(value.MemberBegin())
			{
				return value.MemberBegin();
			}

			iterator end() -> decltype(value.MemberEnd())
			{
				return value.MemberEnd();
			}
		};
		struct object_type : value_type
		{
			object_type() : value_type(rapidjson::kObjectType) {}
			object_type(const object_type&) = default;
			object_type& operator =(const object_type&) = default;
			object_type(object_type&&) = default;
			object_type& operator =(object_type&&) = default;

			value_type& operator[](const std::string& key)
			{
				auto member = value.FindMember(value_type(key.c_str()));
				if (member != value.MemberEnd())
					return *member;
				else
					return value.AddMember(value_type(key.c_str()), value_type(), Allocator()); // TODO: Fix using same allocator
			}
		};

		static json::type get_type(const value_type& val) {
			using json::type;
			if (val.IsBool()) return type::boolean;
			if (val.IsInt64()) return type::integer;
			if (val.IsDouble()) return type::number;
			if (val.IsString()) return type::string;
			if (val.IsArray()) return type::array;
			if (val.IsObject()) return type::object;

			throw std::logic_error("invalid type");
		}

		static object_type as_object(const value_type& val) {
			if (!val.IsObject())
				throw std::bad_cast();
			return val;
		}

		static string_type as_string(const value_type& val) {
			if (!val.IsString())
				throw std::bad_cast();
			return val.GetString();
		}

		static array_type as_array(const value_type& val) {
			if (!val.IsArray())
				throw std::bad_cast();
			return val.GetArray();
		}

		static integer_type as_int(const value_type& val) {
			if (!val.IsInt64())
				throw std::bad_cast();
			return val.GetInt64();
		}

		static boolean_type as_bool(const value_type& val) {
			if (!val.IsBool())
				throw std::bad_cast();
			return val.GetBool();
		}

		static double as_number(const value_type& val) {
			if (!val.IsDouble())
				throw std::bad_cast();
			return val.GetDouble();
		}

		static value_type from_object(const object_type& obj) {
			return obj;
		}

		static value_type from_string(const string_type& obj) {
			return obj.c_str();
		}

		static value_type from_array(const array_type& obj) {
			return obj;
		}

		static value_type from_int(const integer_type& obj) {
			return obj;
		}

		static value_type from_bool(const boolean_type& obj) {
			return obj;
		}

		static value_type from_number(const number_type& obj) {
			return obj;
		}

		static bool parse(document_type& document, const std::string& str)
		{
			return !document.Parse(str).HasParseError();
		}

		static std::string serialize(const document_type& document){
			rapidjson::GenericStringBuffer<Encoding> buffer;
			rapidjson::Writer<rapidjson::GenericStringBuffer<Encoding>, Encoding, Encoding, StackAllocator> writer(buffer);
			document.Accept(writer);

			return buffer.GetString();
		}
	};
	
	using default_rapidjson_traits = rapidjson_traits<rapidjson::UTF8<>>;

	/// Default JSON claim
	/** This type is the default specialization of the \ref basic_claim class which
	  * uses the standard template types.
	  */
	using claim = basic_claim<default_rapidjson_traits>;

	/// Create a verifier using the default clock
	/** \return verifier instance
	  */
	inline
		verifier<default_clock, default_rapidjson_traits> verify() {
		return verify<default_clock, default_rapidjson_traits>(default_clock{});
	}
	/// Return a picojson builder instance to create a new token
	inline builder<default_rapidjson_traits> create()
	{
		return builder<default_rapidjson_traits>();
	}

	/// Decode a token
	/** \param token Token to decode
	  * \return Decoded token
	  * \throw std::invalid_argument Token is not in correct format
	  * \throw std::runtime_error Base64 decoding failed or invalid json
	  */
	inline decoded_jwt<default_rapidjson_traits> decode(const std::string& token)
	{
		return decoded_jwt<default_rapidjson_traits>(token);
	}
}  // namespace jwt


TEST(RapidJsonTest, BasicClaims) {
	using rapidjson_claim = jwt::basic_claim<jwt::default_rapidjson_traits>;

	const auto string = rapidjson_claim(std::string("string"));
	const auto array = rapidjson_claim(std::set<std::string>{"string", "string"});
	const auto integer = rapidjson_claim(jwt::default_rapidjson_traits::value_type(159816816));
}

TEST(RapidJsonTest, AudienceAsString) {

	std::string token =
			"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhdWQiOiJ0ZXN0In0."
			"WZnM3SIiSRHsbO3O7Z2bmIzTJ4EC32HRBKfLznHhrh4";
	auto decoded = jwt::decode<jwt::default_rapidjson_traits>(token);

	ASSERT_TRUE(decoded.has_algorithm());
	ASSERT_TRUE(decoded.has_type());
	ASSERT_FALSE(decoded.has_content_type());
	ASSERT_FALSE(decoded.has_key_id());
	ASSERT_FALSE(decoded.has_issuer());
	ASSERT_FALSE(decoded.has_subject());
	ASSERT_TRUE(decoded.has_audience());
	ASSERT_FALSE(decoded.has_expires_at());
	ASSERT_FALSE(decoded.has_not_before());
	ASSERT_FALSE(decoded.has_issued_at());
	ASSERT_FALSE(decoded.has_id());

	ASSERT_EQ("HS256", decoded.get_algorithm());
	ASSERT_EQ("JWT", decoded.get_type());
	auto aud = decoded.get_audience();
	ASSERT_EQ(1, aud.size());
	ASSERT_EQ("test", *aud.begin());
}

TEST(RapidJsonTest, SetArray) {
	std::vector<int64_t> vect = {
		100,
		20,
		10
	};
	auto token = jwt::create<jwt::default_rapidjson_traits>()
		.set_payload_claim("test", jwt::basic_claim<jwt::default_rapidjson_traits>(vect.begin(), vect.end()))
		.sign(jwt::algorithm::none{});
	ASSERT_EQ(token, "eyJhbGciOiJub25lIn0.eyJ0ZXN0IjpbMTAwLDIwLDEwXX0.");
}

TEST(RapidJsonTest, SetObject) {
	std::istringstream iss{"{\"api-x\": [1]}"};
	jwt::basic_claim<jwt::default_rapidjson_traits> object;
	iss >> object;
	ASSERT_EQ(object.get_type() , jwt::json::type::object);

	auto token = jwt::create<jwt::default_rapidjson_traits>()
		.set_payload_claim("namespace", std::move(object))
		.sign(jwt::algorithm::hs256("test"));
	ASSERT_EQ(token, "eyJhbGciOiJIUzI1NiJ9.eyJuYW1lc3BhY2UiOnsiYXBpLXgiOlsxXX19.F8I6I2RcSF98bKa0IpIz09fRZtHr1CWnWKx2za-tFQA");
}

TEST(RapidJsonTest, VerifyTokenHS256) {
	std::string token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXUyJ9.eyJpc3MiOiJhdXRoMCJ9.AbIJTDMFc7yUa5MhvcP03nJPyCPzZtQcGEp-zWfOkEE";

	auto verify = jwt::verify<jwt::default_clock, jwt::default_rapidjson_traits>({})
		.allow_algorithm(jwt::algorithm::hs256{ "secret" })
		.with_issuer("auth0");

	auto decoded_token = jwt::decode<jwt::default_rapidjson_traits>(token);
	verify.verify(decoded_token);
}
