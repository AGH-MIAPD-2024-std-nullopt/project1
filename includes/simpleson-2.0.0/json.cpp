/*! \file json.cpp
 * \brief Simpleson source file
 */

#include "json.h"
#include <string.h>
#include <assert.h>

/*! \brief Checks for an empty string
 * 
 * @param str The string to check
 * @return True if the string is empty, false if the string is not empty
 * @warning The string must be null-terminated for this macro to work
 */
#define EMPTY_STRING(str) (*str == '\0')

/*! \brief Moves a pointer to the first character that is not white space
 *
 * @param str The pointer to move
 */
#define SKIP_WHITE_SPACE(str) { const char *next = json::parsing::tlws(str); str = next; }

/*! \brief Determines if the supplied character is a digit
 *
 * @param input The character to be tested
 */
#define IS_DIGIT(input) (input >= '0' && input <= '9')

/*! \brief Format used for character to string conversion */
const char * CHAR_FORMAT = "%c";

/*! \brief Format used for floating-point number to string conversion */
const char * FLOAT_FORMAT = "%f";

/*! \brief Format used for double floating-opint number to string conversion */
const char * DOUBLE_FORMAT = "%lf";

namespace json
{
    typedef std::pair<std::string, json::data_reference> kvp;
    
    /*! \brief Namespace used for JSON parsing functions */
	namespace parsing
	{
		/*! \brief (t)rims (l)eading (w)hite (s)pace
		 *
		 * \details Given a string, returns a pointer to the first character that is not white space. Spaces, tabs, and carriage returns are all considered white space. 
		 * @param start The string to examine
		 * @return A pointer within the input string that points at the first charactor that is not white space
		 * \note If the string consists of entirely white space, then the null terminator is returned
		 * \warning The behavior of this function with string that is not null-terminated is undefined
		 */
		const char* tlws(const char *start);

		/*! \brief Decodes a string in JSON format
		 *
		 * \details The quotation mark ("), reverse solidus (\), solidus (/), backspace (b), formfeed (f), linefeed (n), carriage return (r), horizontal tab (t), and Unicode character will be unescaped
		 * @param input A string, encapsulated in quotations ("), potentially containing escaped control characters
		 * @return A string with control characters un-escaped
		 * \note This function will strip leading and trailing quotations. 
		 * @see encode_string
		 */
		std::string decode_string(const char * input);

		/*! \brief Encodes a string in JSON format
		 *
		 * \details The quotation mark ("), reverse solidus (\), solidus (/), backspace (b), formfeed (f), linefeed (n), carriage return (r), horizontal tab (t), and Unicode character will be escaped
		 * @param input A string potentially containing control characters
		 * @return A string that has all control characters escaped with a reverse solidus (\)
		 * \note This function will add leading and trailing quotations. 
		 * @see decode_string
		 */
		std::string encode_string(const char *input);

		/*! \brief Structure for capturing the results of parsing */
		struct parse_results
		{
			/*! \brief The type of value encountered while parsing */
			jtype::jtype type; 

			/*! \brief The parsed value encountered */
			std::string value; 

			/*! \brief A pointer to the first character after the parsed value */
			const char *remainder;
		};

		/*! \brief Parses the first value encountered in a JSON string
		 *
		 * @param input The string to be parsed
		 * @return Details regarding the first value encountered 
		 * \exception json::parsing_error Exception thrown when the input is not valid JSON
		 */
		parse_results parse(const char *input);
		
		/*! \brief Template for reading a numeric value 
		 * 
		 * @tparam T The C data type the input will be convered to
		 * @param input The string to conver to a number
		 * @param format The format to use when converting the string to a number
		 * @return The numeric value contained by the input
		 */
		template <typename T>
		T get_number(const char *input, const char* format)
		{
			T result;
			std::sscanf(input, format, &result);
			return result;
		}

		/*! \brief Converts a number to a string
		 * 
		 * @tparam The C data type of the number to be converted
		 * @param number A reference to the number to be converted
		 * @param format The format to be used when converting the number
		 * @return A string representation of the input number
		 */ 
		template <typename T>
		std::string get_number_string(const T &number, const char *format)
		{
			std::vector<char> cstr(6);
			int remainder = std::snprintf(&cstr[0], cstr.size(), format, number);
			if(remainder < 0) {
				return std::string();
			} else if(remainder >= (int)cstr.size()) {
				cstr.resize(remainder + 1);
				std::snprintf(&cstr[0], cstr.size(), format, number);
			}
			std::string result(&cstr[0]);
			return result;
		}

		/*! \brief Parses a JSON array
		 *
		 * \details Converts a serialized JSON array into a vector of the values in the array
		 * @param input The serialized JSON array
		 * @return A vector containing each element of the array with each element being serialized JSON
		 */
		std::vector<std::string> parse_array(const char *input);
	}
}

namespace
{
    const char * golden_true = "true";
    const char * golden_false = "false";
    const char * golden_null = "null";

    class null_data_source : public json::data_source
    {
    public:
        inline null_data_source() { }
        inline virtual json::jtype::jtype type() const { return json::jtype::jnull; }
        inline virtual std::string serialize() const { return std::string("null"); }
        inline virtual std::string as_string() const { return this->serialize(); }
    };

    template<typename T>
    class number_data_source : public json::data_source
    {
    public:
        virtual json::jtype::jtype type() const { return json::jtype::jnumber; }
        number_data_source(const T value) : __value(value) { }
        virtual operator uint8_t() const { return (uint8_t)this->__value; }
        virtual operator int8_t() const { return (int8_t)this->__value; }
        virtual operator uint16_t() const { return (uint16_t)this->__value; }
        virtual operator int16_t() const { return (int16_t)this->__value; }
        virtual operator uint32_t() const { return (uint32_t)this->__value; }
        virtual operator int32_t() const { return (int32_t)this->__value; }
        virtual operator uint64_t() const { return (uint64_t)this->__value; }
        virtual operator int64_t() const { return (int64_t)this->__value; }
        virtual operator float() const { return (float)this->__value; }
        virtual operator double() const { return (double)this->__value; }
        virtual operator long double() const { return (long double)this->__value; }

        virtual std::string serialize() const { return this->as_string(); }
    protected:
        const T __value;
    };

    template<typename T>
    class integer_data_source : public number_data_source<T>
    {
    public:
        integer_data_source(const T value) : number_data_source<T>(value) { }
        virtual bool as_bool() const { return this->__value != 0; }
        virtual std::string as_string() const
        {
            if(this->__value == 0) return "0";
            T remainder = this->__value;
            std::string result;
            while(remainder != 0)
            {
                const char digit = (remainder % 10) + '0';
                result.insert(result.begin(), digit);
                remainder /= 10;
            }
            if(this->__value < 0) result.insert(result.begin(), '-');
            return result;
        }
    };

    template<typename T>
    class floating_point_data_source : public number_data_source<T>
    {
    public:
        floating_point_data_source(const T value, const char * format) 
            : number_data_source<T>(value),
            __format(format),
            __cache(NULL)
        { }

        floating_point_data_source(const floating_point_data_source &other)
            : number_data_source<T>(other.__value),
            __format(other.__format),
            __cache(other.__cache)
        { }

        floating_point_data_source(const std::string &value, const char * format)
            : number_data_source<T>(0),
            __format(format),
            __cache(NULL)
        {
            std::sscanf(value.c_str(), format, &this->__value);
            this->__cache = new std::string(value);
        }

        virtual ~floating_point_data_source()
        {
            if(this->__cache != NULL) delete this->__cache;
        }

        virtual std::string as_string() const
        {
            if(this->__cache != NULL) return *this->__cache;
            std::vector<char> cstr(6);
            int remainder = std::snprintf(&cstr[0], cstr.size(), this->__format, this->__value);
            if(remainder < 0) {
                throw std::bad_cast();
            } else if(remainder >= (int)cstr.size()) {
                cstr.resize(remainder + 1);
                std::snprintf(&cstr[0], cstr.size(), this->__format, this->__value);
            }
            return std::string(&cstr[0]);
        }
    private:
        const char * __format;
        const std::string * __cache;
    };

    class string_data_source : public json::data_source, private std::string
    {
    public:
        inline string_data_source(const std::string &value, bool encoded) 
            : std::string(encoded ? json::parsing::decode_string(value.c_str()) : value) 
        {
            assert(value.length() > (encoded ? 2 : 1));
            assert(!encoded || (value.at(0) == '"' && value.at(value.length() - 1) == '"'));
        }
        inline virtual json::jtype::jtype type() const { return json::jtype::jstring; }
        inline virtual std::string serialize() const { return json::parsing::encode_string(this->c_str()); }
        inline virtual std::string as_string() const { return *this; }
    };

    template<typename T, json::jtype::jtype __type>
    class json_data_source : public json::data_source
    {
    public:
        T data;
        inline json_data_source() : data() { }
        inline json_data_source(const T &value) : data(value) { }
        inline virtual json::jtype::jtype type() const { return __type; }
        inline virtual std::string serialize() const { return this->data.serialize(); }
        inline virtual std::string as_string() const { return this->data.serialize(); }
        inline virtual operator T() const { return this->data; }
    };

    typedef json_data_source<json::jarray, json::jtype::jarray> jarray_data_source;

    typedef json_data_source<json::jobject, json::jtype::jobject> jobject_data_source;

    class bool_data_source : public json::data_source
    {
    public:
        inline bool_data_source(const bool value) : __data(value) { }
        inline virtual json::jtype::jtype type() const { return json::jtype::jbool; }
        virtual std::string serialize() const { return this->__data ? std::string("true") : std::string("false"); }
        inline virtual json::data_source * copy() const { return new bool_data_source(this->__data); }

        virtual inline operator uint8_t() const { return (uint8_t)this->__data; }
        virtual inline operator int8_t() const { return (int8_t)this->__data; }
        virtual inline operator uint16_t() const { return (uint16_t)this->__data; }
        virtual inline operator int16_t() const { return (int16_t)this->__data; }
        virtual inline operator uint32_t() const { return (uint32_t)this->__data; }
        virtual inline operator int32_t() const { return (int32_t)this->__data; }
        virtual inline operator uint64_t() const { return (uint64_t)this->__data; }
        virtual inline operator int64_t() const { return (int64_t)this->__data; }
        virtual inline operator float() const { return (float)this->__data; }
        virtual inline operator double() const { return (double)this->__data; }

        virtual inline std::string as_string() const { return this->serialize(); }
        virtual bool as_bool() const { return this->__data; }
    private:
        bool __data;
    };

    bool is_control_character(const char input)
    {
        switch (input)
        {
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case '"':
        case '\\':
        case '/':
            return true;
        default:
            return false;
        }
    }

    bool is_hex_digit(const char input)
    {
        return IS_DIGIT(input) || (input >= 'a' && input <= 'f') || (input >= 'A' && input <= 'F');
    }

    class string_parser : public json::data_parser, private std::string
    {
    private:
        enum state_enum
		{
			STRING_EMPTY = 0, ///< No values have been read
			STRING_OPENING_QUOTE, ///< The opening quote has been read. Equivalant to #STRING_OPEN, but used for debugging the state
			STRING_OPEN, ///< The opening quote has been read and the last character was not an escape character
			STRING_ESCAPED, ///< The last character was an reverse solidus (\), indicating the next character should be a control character 
			STRING_CODE_POINT_START, ///< An encoded unicode character is encountered. Expecting four following hex digits. 
			STRING_CODE_POINT_1, ///< An encoded unicode character is encountered. Expecting three following hex digits (one has already been read). 
			STRING_CODE_POINT_2, ///< An encoded unicode character is encountered. Expecting two following hex digits (two have already been read). 
			STRING_CODE_POINT_3, ///< An encoded unicode character is encountered. Expecting one following hex digit (three has already been read). 
			STRING_CLOSED ///< The closing quote has been read. Reading should cease. 
		};

        state_enum __state;
    public:
        virtual json::jtype::jtype type() const { return json::jtype::jstring; }

        virtual push_result push(const char next)
        {
            switch (this->__state)
            {
            case STRING_EMPTY:
                assert(this->length() == 0);
                if(next == '"') {
                    assert(this->length() == 0);
                    this->push_back(next);
                    this->__state = STRING_OPENING_QUOTE;
                    return ACCEPTED;
                }
                return REJECTED;
            case STRING_OPENING_QUOTE:
                assert(this->length() == 1);
                this->__state = STRING_OPEN;
                // Fall through deliberate
            case STRING_OPEN:
                assert(this->length() > 0);
                switch (next)
                {
                case '\\':
                    this->__state = STRING_ESCAPED;
                    break;
                case '"':
                    this->__state = STRING_CLOSED;
                    break;
                default:
                    // No state change
                    break;
                }
                this->push_back(next);
                return ACCEPTED;
            case STRING_ESCAPED:
                if(is_control_character(next)) {
                    this->__state = STRING_OPEN;
                    this->push_back(next);
                    return ACCEPTED;
                } else if(next == 'u') {
                    this->__state = STRING_CODE_POINT_START;
                    this->push_back(next);
                    return ACCEPTED;
                }
                return REJECTED;
            case STRING_CODE_POINT_START:
                assert(this->back() == 'u');
                if(!is_hex_digit(next)) return REJECTED;
                this->push_back(next);
                this->__state = STRING_CODE_POINT_1;
                return ACCEPTED;
            case STRING_CODE_POINT_1:
                assert(is_hex_digit(this->back()));
                if(!is_hex_digit(next)) return REJECTED;
                this->push_back(next);
                this->__state = STRING_CODE_POINT_2;
                return ACCEPTED;
            case STRING_CODE_POINT_2:
                assert(is_hex_digit(this->back()));
                if(!is_hex_digit(next)) return REJECTED;
                this->push_back(next);
                this->__state = STRING_CODE_POINT_3;
                return ACCEPTED;
            case STRING_CODE_POINT_3:
                assert(is_hex_digit(this->back()));
                if(!is_hex_digit(next)) return REJECTED;
                this->push_back(next);
                this->__state = STRING_OPEN;
                return ACCEPTED;
            case STRING_CLOSED:
                return REJECTED;
            }
            throw std::logic_error("Unexpected return");
        }

        virtual bool is_valid() const { return this->__state == STRING_CLOSED; }

		virtual void reset()
        {
            this->clear();
            this->__state = STRING_EMPTY;
        }

        virtual json::data_reference emit() const
        {
            if(!this->is_valid()) throw std::bad_cast();
            return json::data_reference::create(new string_data_source(*this, true));
        }
    };

    bool is_integer_string(const std::string &input)
    {
        assert(input.size() > 0);
        size_t start = 0;
        if(input.at(0) == '-')
        {
            start++;
            assert(input.size() > 1);
        }
        for(size_t i = start; i < input.size(); i++)
        {
            if(!IS_DIGIT(input.at(i))) return false;
        }
        return true;
    }

    json::data_source * number_from_string(const std::string &input)
    {
        assert(input.size() > 0);
        if(is_integer_string(input)) {
            // TODO: Check for overflow
            
            if(input.at(0) == '-') {
                assert(input.size() > 1);
                int64_t value = input.at(1) - '0';
                value *= -1;
                for(size_t i = 1; i < input.size(); i++) {
                    value *= 10;
                    const int64_t next = input.at(i) - '0';
                    if(value - next > value) throw std::overflow_error("int too large");
                    value -= next;
                }
                return new integer_data_source<int64_t>(value);
            }

            uint64_t value = input.at(0) - '0';
            for(size_t i = 1; i < input.size(); i++) {
                value *= 10;
                const uint64_t next = input.at(i) - '0';
                if(value + next < value) throw std::overflow_error("int too large");
                value += next;
            }
            return new integer_data_source<uint64_t>(value);
        }

        // Input is a float
        return new floating_point_data_source<double>(input, DOUBLE_FORMAT);
    }

    class number_parser : public json::data_parser, private std::string
    {
    private:
        enum state_enum
		{
			NUMBER_EMPTY = 0, ///< No values have been read
			NUMBER_OPEN_NEGATIVE, ///< A negative value has been read as the first character
			NUMBER_ZERO, ///< A zero has been read as an integer value
			NUMBER_INTEGER_DIGITS, ///< Integer digits were the last values read
			NUMBER_DECIMAL, ///< A decimal point was the last value read
			NUMBER_FRACTION_DIGITS, ///< A decimal point and subsequent digits were the last values read
			NUMBER_EXPONENT, ///< An exponent indicator has been read
			NUMBER_EXPONENT_SIGN, ///< An exponent sign has been read
			NUMBER_EXPONENT_DIGITS ///< An exponent indicator and subsequent digits were the last values read
		};

        state_enum __state;
    public:
        virtual json::jtype::jtype type() const { return json::jtype::jstring; }

        virtual push_result push(const char next)
        {
            switch (this->__state)
            {
            case NUMBER_EMPTY:
                assert(this->length() == 0);
                if(next == '-') {
                    this->__state = NUMBER_OPEN_NEGATIVE;
                    this->push_back(next);
                    return ACCEPTED;
                } else if(IS_DIGIT(next)) {
                    this->__state = next == '0' ? NUMBER_ZERO : NUMBER_INTEGER_DIGITS;
                    this->push_back(next);
                    return ACCEPTED;
                }
                return REJECTED;
            case NUMBER_OPEN_NEGATIVE:
                if(IS_DIGIT(next)) {
                    this->__state = next == '0' ? NUMBER_ZERO : NUMBER_INTEGER_DIGITS;
                    this->push_back(next);
                    return ACCEPTED;
                }
                return REJECTED;
            case NUMBER_INTEGER_DIGITS:
                assert(IS_DIGIT(this->back()));
                if(IS_DIGIT(next)) {
                    this->push_back(next);
                    return ACCEPTED;
                }
                // Fall-through deliberate
            case NUMBER_ZERO:
                switch (next)
                {
                case '.':
                    this->__state = NUMBER_DECIMAL;
                    this->push_back(next);
                    return ACCEPTED;
                case 'e':
                case 'E':
                    this->__state = NUMBER_EXPONENT;
                    this->push_back(next);
                    return ACCEPTED;
                default:
                    return REJECTED;
                }
            case NUMBER_DECIMAL:
                assert(this->back() == '.');
                if(IS_DIGIT(next)) {
                    this->__state = NUMBER_FRACTION_DIGITS;
                    this->push_back(next);
                    return ACCEPTED;
                }
                return REJECTED;
            case NUMBER_FRACTION_DIGITS:
                assert(IS_DIGIT(this->back()));
                if(IS_DIGIT(next)) {
                    this->push_back(next);
                    return ACCEPTED;
                } else if(next == 'e' || next == 'E') {
                    this->__state = NUMBER_EXPONENT;
                    this->push_back(next);
                    return ACCEPTED;
                }
                return REJECTED;
            case NUMBER_EXPONENT:
                assert(this->back() == 'e' || this->back() == 'E');
                if(next == '+' || next == '-') {
                    this->__state = NUMBER_EXPONENT_SIGN;
                    this->push_back(next);
                    return ACCEPTED;
                }
                // Fall-through deliberate
            case NUMBER_EXPONENT_SIGN:
            case NUMBER_EXPONENT_DIGITS:
                if(IS_DIGIT(next)) {
                    this->__state = NUMBER_EXPONENT_DIGITS;
                    this->push_back(next);
                    return ACCEPTED;
                }
                return REJECTED;
            }
            throw std::logic_error("Unexpected return");
        }

        virtual bool is_valid() const
        {
            switch (this->__state)
            {
            case NUMBER_ZERO:
            case NUMBER_INTEGER_DIGITS:
            case NUMBER_FRACTION_DIGITS:
            case NUMBER_EXPONENT_DIGITS:
                return true;
            default:
                return false;
            }
        }

		virtual void reset()
        {
            this->clear();
            this->__state = NUMBER_EMPTY;
        }

        virtual json::data_reference emit() const
        {
            switch (this->__state)
            {
            case NUMBER_ZERO:
            case NUMBER_INTEGER_DIGITS:
            case NUMBER_FRACTION_DIGITS:
            case NUMBER_EXPONENT_DIGITS:
                return json::data_reference::create(number_from_string(*this));
            default:
                break;
            }
            throw std::bad_cast();
        }
    };

    class boolean_parser : public json::data_parser
    {
    private:
        bool __value;
        size_t __bytes_read;

    public:
        boolean_parser() : __value(false), __bytes_read(0) { }

        virtual json::jtype::jtype type() const { return json::jtype::jbool; }

        virtual push_result push(const char next)
        {
            const char * golden = this->__value ? golden_true : golden_false;

            if(this->is_valid()) return REJECTED;

            assert(this->__bytes_read < strlen(golden));

            if(this->__bytes_read == 0)
            {
                if(std::isspace(next)) return WHITESPACE;
                switch (next)
                {
                case 't':
                    this->__value = true;
                    this->__bytes_read++;
                    return ACCEPTED;
                    break;
                case 'f':
                    this->__value = false;
                    this->__bytes_read++;
                    return ACCEPTED;
                default:
                    return REJECTED;
                    break;
                }
            }

            if(next == golden[this->__bytes_read]) {
                this->__bytes_read++;
                return ACCEPTED;
            }

            return REJECTED;
        }

        virtual bool is_valid() const
        {
            return this->__bytes_read == 
                ( this->__value 
                ? 
                strlen(golden_true)
                :
                strlen(golden_false)
                );
        }

		virtual void reset()
        {
            this->__bytes_read = 0;
        }

        virtual json::data_reference emit() const
        {
            return json::data_reference::create(new bool_data_source(this->__value));
        }
    };

    class null_parser : public json::data_parser
    {
    private:
        bool __value;
        size_t __bytes_read;

    public:
        null_parser() : __bytes_read(0) { }

        virtual json::jtype::jtype type() const { return json::jtype::jnull; }

        virtual push_result push(const char next)
        {
            if(this->is_valid()) return REJECTED;

            assert(this->__bytes_read < strlen(golden_null));

            if(next == golden_null[this->__bytes_read]) {
                this->__bytes_read++;
                return ACCEPTED;
            }

            return REJECTED;
        }

        virtual bool is_valid() const
        {
            return this->__bytes_read == strlen(golden_null);
        }

		virtual void reset()
        {
            this->__bytes_read = 0;
        }

        virtual json::data_reference emit() const
        {
            return json::data_reference::create(new null_data_source());
        }
    };

    class jarray_parser : public json::jarray::istream
    {
    public:
        inline jarray_parser(json::jarray * sink) 
            : json::jarray::istream(),
            __obj(sink)
        { assert(sink != NULL); }
        virtual void on_array_opened()
        {
            this->__obj->clear();
        }
        virtual void on_value_read(const json::data_reference &value)
        {
            this->__obj->push_back(value);
        }
        virtual void on_array_closed()
        {
            // Do nothing
        }
    private:
        json::jarray * __obj;
    };

    class jobject_parser : public json::jobject::istream
    {
    public:
        inline jobject_parser(json::jobject * sink) 
            : json::jobject::istream(),
            __obj(sink)
        { assert(sink != NULL); }
        virtual void on_object_opened()
        {
            this->__obj->clear();
        }
        virtual void on_key_read(const std::string &key, const json::jtype::jtype type)
        { 
            // Do nothing
        }
        virtual void on_value_read(const std::string &key, const json::data_reference &value)
        {
            this->__obj->set(key, value);
        }
        virtual void on_object_closed()
        {
            // Do nothing
        }
    private:
        json::jobject * __obj;
    };

    json::data_parser * create_parser(const json::jtype::jtype type)
    {
        switch (type)
        {
        case json::jtype::jarray:
            return new json::jarray::parser();
            break;
        case json::jtype::jbool:
            return new boolean_parser();
            break;
        case json::jtype::jnull:
            return new null_parser();
            break;
        case json::jtype::jnumber:
            return new number_parser();
            break;
        case json::jtype::jobject:
            return new json::jobject::parser();
            break;
        case json::jtype::jstring:
            return new string_parser();
            break;
        case json::jtype::not_valid:
            return NULL;
            break;
        }
    }
}

const char* json::parsing::tlws(const char *input)
{
    const char *output = input;
    while(!EMPTY_STRING(output) && std::isspace(*output)) output++;
    return output;
}

json::jtype::jtype json::jtype::peek(const char input)
{
    switch (input)
    {
    case '[':
        return json::jtype::jarray;
    case '"':
        return json::jtype::jstring;
    case '{':
        return json::jtype::jobject;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return json::jtype::jnumber;
    case 't':
    case 'f':
        return json::jtype::jbool;
    case 'n':
        return json::jtype::jnull;
    default:
        return json::jtype::not_valid;
    }
}

json::data_source::data_source()
{ }

json::data_source::~data_source()
{ }

json::data_source::operator json::jarray() const
{
    throw std::bad_cast();
}

json::data_source::operator json::jobject() const
{
    throw std::bad_cast();
}

json::jarray json::data_source::as_array() const
{
    return this->operator json::jarray();
}

json::jobject json::data_source::as_object() const
{
    return this->operator json::jobject();
}

json::data_reference::operator json::jarray() const
{
    return this->__source->operator json::jarray();
}

json::data_reference::operator json::jobject() const
{
    return this->__source->operator json::jobject();
}

std::string json::parsing::decode_string(const char *input)
{
    const char *index = input;
    std::string result;

    if(*index != '"') throw json::parsing_error("Expecting opening quote");
    index++;
    bool escaped = false;
    // Loop until the end quote is found
    while(!(!escaped && *index == '"'))
    {
        if(escaped)
        {
            switch (*index)
            {
            case '"':
            case '\\':
            case '/':
                result += *index;
                break;
            case 'b':
                result += '\b';
                break;
            case 'f':
                result += '\f';
                break;
            case 'n':
                result += '\n';
                break;
            case 'r':
                result += '\r';
                break;
            case 't':
                result += '\t';
                break;
            case 'u':
                // #todo Unicode support
                index += 4;
                break;
            default:
                throw json::parsing_error("Expected control character");
            }
            escaped = false;
        } else if(*index == '\\') {
            escaped = true;
        } else {
            result += *index;
        }
        index++;
    }
    return result;
}

std::string json::parsing::encode_string(const char *input)
{
    std::string result = "\"";

    while (!EMPTY_STRING(input))
    {
        switch (*input)
        {
        case '"':
        case '\\':
        case '/':
            result += "\\";
            result += *input;
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result += *input;
            break;
        }
        input++;
    }
    result += '\"';
    return result;
}

json::jistream::jistream()
{ }

json::jistream::~jistream()
{ }

json::data_parser::data_parser()
{ }

json::data_parser::~data_parser()
{ }

json::jarray json::jarray::parse(const char *input)
{
    // Check for valid input
    if(input == NULL) throw std::invalid_argument(__FUNCTION__);
    SKIP_WHITE_SPACE(input);
    json::jarray::parser sink;
    while(sink.push(*input) != json::jistream::REJECTED)
    {
        input++;
    }
    if(!sink.is_valid()) throw json::unexpected_character(*input);
    return sink.result();
}

std::string json::jarray::as_string() const
{
    if(this->size() == 0) return "[]";
    std::string result = "[";
    for(size_t i = 0; i < this->size() - 1; i++)
    {
        result.append(this->at(i).serialize());
        result.push_back(',');
    }
    result.append(this->back().serialize());
    result.push_back(']');
    return result;
}

json::jarray::istream::istream()
    : __value(NULL),
    __state(INTIALIZED),
    __bytes_accepted(0)
{ }

bool json::jarray::istream::is_valid() const
{
    return this->__state == CLOSED;
}

void json::jarray::istream::reset()
{
    if(this->__value != NULL) {
        delete this->__value;
        this->__value = NULL;
    }
    this->__bytes_accepted = 0;
    this->__state = INTIALIZED;
}

json::jistream::push_result json::jarray::istream::push(const char next)
{
    switch (this->__state)
    {
    case INTIALIZED:
        if(std::isspace(next)) return json::jistream::WHITESPACE;
        if(next == '[') {
            this->__bytes_accepted = 1;
            this->on_array_opened();
            this->__state = OPENED;
            return json::jistream::ACCEPTED;
        }
        return json::jistream::REJECTED;
        break;
    case OPENED:
        if(next == ']') {
            this->__bytes_accepted++;
            this->__state = CLOSED;
            return json::jistream::ACCEPTED;
        }
        // Fall-through
    case AWAITING_VALUE:
        if(std::isspace(next)) return WHITESPACE;
        assert(this->__value == NULL);
        this->__value = create_parser(json::jtype::peek(next));
        if(this->__value == NULL) return REJECTED;
        switch (this->__value->push(next))
        {
        case ACCEPTED:
            this->__bytes_accepted++;
            this->__state = READING_VALUE;
            return ACCEPTED;
            break;
        case REJECTED:
            return REJECTED;
        default:
            throw std::logic_error("Unexpected return");
            break;
        }
        break;
    case READING_VALUE:
        switch (this->__value->push(next))
        {
        case ACCEPTED:
            this->__bytes_accepted++;
            return ACCEPTED;
            break;
        case WHITESPACE:
            return WHITESPACE;
            break;
        case REJECTED:
            if(!this->__value->is_valid()) return REJECTED;
            this->on_value_read(this->__value->emit());
            delete this->__value;
            this->__value = NULL;
            break;
            // Fall-through
        }
        // Fall-through
    case VALUE_READ:
        if(std::isspace(next)) {
            this->__state = VALUE_READ;
            return WHITESPACE;
        } else if(next == ',') {
            this->__bytes_accepted++;
            this->__state = AWAITING_VALUE;
            return ACCEPTED;
        } else if(next == ']') {
            this->__bytes_accepted++;
            this->on_array_closed();
            this->__state = CLOSED;
            return ACCEPTED;
        }
        return REJECTED;
        break;
    case CLOSED:
        return REJECTED;
    }
}

json::jarray::parser::parser()
    : __handler(NULL)
{
    this->reset();
    this->__handler = new jarray_parser(this->__data);
}

json::jarray::parser::~parser()
{
    this->reset();
    delete this->__handler;
}

void json::jarray::parser::reset()
{
    jarray_data_source * source = new jarray_data_source();
    this->__source = source;
    this->__data = &source->data;
    this->__ref = json::data_reference::create(this->__source);
    if(this->__handler != NULL)
        this->__handler->reset();
}

const json::jarray& json::jarray::parser::result() const
{
    if(!this->is_valid()) throw std::bad_cast();
    assert(this->__data != NULL);
    return *this->__data;
}

json::data_reference json::jarray::parser::emit() const
{
    if(!this->is_valid()) throw std::bad_cast();
    return this->__ref;
}

json::jobject json::jobject::parse(const char *input)
{
    // Check for valid input
    if(input == NULL) throw std::invalid_argument(__FUNCTION__);
    SKIP_WHITE_SPACE(input);
    json::jobject::parser sink;
    while(sink.push(*input) != json::jistream::REJECTED)
    {
        input++;
    }
    if(!sink.is_valid()) throw json::unexpected_character(*input);
    return sink.result();
}

json::key_list_t json::jobject::list_keys() const
{
    // Initialize the result
    key_list_t result;

    for (json::jmap::const_iterator it = this->begin(); it != this->end(); ++it)
    {
        result.push_back(it->first);
    }
    return result;
}

void json::jobject::set(const std::string &key, const json::data_reference &value)
{
    json::jmap::iterator it = this->find(key);
    if(it != this->end()) {
        it->second = value;
    } else {
        this->insert(kvp(key, value));
    }
}

void json::jobject::remove(const std::string &key)
{
    this->erase(key);
}

json::jobject::operator std::string() const
{
    if (this->size() == 0) return "{}";
    std::string result = "{";
    for (json::jmap::const_iterator it = this->begin(); it != this->end(); ++it)
    {
        result += json::parsing::encode_string(it->first.c_str()) + ":" + it->second.serialize() + ",";
    }
    result.erase(result.size() - 1, 1);
    result += "}";
    return result;
}

std::string json::jobject::pretty(unsigned int indent_level) const
{
    std::string result = "";
    for(unsigned int i = 0; i < indent_level; i++) result += "\t";
    if(this->size() == 0) {
        result += "{}";
        return result;
    }
    result += "{\n";
    for (json::jmap::const_iterator it = this->begin(); it != this->end(); ++it)
    {
        for(unsigned int j = 0; j < indent_level + 1; j++) result += "\t";
        result += "\"" + it->first + "\": ";
        switch(it->second.type()) {
            case json::jtype::jarray:
                result += std::string(json::parsing::tlws(it->second.as_array().pretty(indent_level + 1).c_str()));
                break;
            case json::jtype::jobject:
                result += std::string(json::parsing::tlws(it->second.as_object().pretty(indent_level + 1).c_str()));
                break;
            default:
                result += it->second.serialize();
                break;
        }

        result += ",\n";
    }
    result.erase(result.size() - 2, 1);
    for(unsigned int i = 0; i < indent_level; i++) result += "\t";
    result += "}";
    return result;
}

template<typename T>
T cast_uint(const std::string &input, const T max_value)
{
    const char *overflow = __FUNCTION__; 
    assert(input.size() > 0);
    if(input == "0") return 0;
    T result = input.at(0) - (T)'0';
    const T ninety_percent = max_value / 10;
    for(size_t i = 1; i < input.size(); i++)
    {
        if(result > ninety_percent) throw std::overflow_error(overflow);
        result *= 10;
        assert(IS_DIGIT(input.at(i)));
        const T digit = input.at(i) - (T)'0';
        if(result > max_value - digit) throw std::overflow_error(overflow);
        result += digit;
    }
    return result;
}

std::string json::jarray::pretty(unsigned int indent_level) const
{
    std::string result = "";
    for(unsigned int i = 0; i < indent_level; i++) result += "\t";
    if(this->size() == 0) {
        result += "[]";
        return result;
    }
    result += "[\n";
    for (size_t i = 0; i < this->size(); i++)
    {
        switch(json::jtype::peek(*this->at(i).as_string().c_str())) {
            case json::jtype::jarray:
                result += json::jarray::parse(this->at(i).as_string()).pretty(indent_level + 1);
                break;
            case json::jtype::jobject:
                result += json::jobject::parse(this->at(i).as_string()).pretty(indent_level + 1);
                break;
            default:
                for(unsigned int j = 0; j < indent_level + 1; j++) result += "\t";
                result += this->at(i).serialize();
                break;
        }

        result += ",\n";
    }
    result.erase(result.size() - 2, 1);
    for(unsigned int i = 0; i < indent_level; i++) result += "\t";
    result += "]";
    return result;
}

json::proxy::proxy()
    : json::dynamic_data(),
    __parent(NULL)
{ }

json::proxy::proxy(const json::proxy &other)
    : json::dynamic_data(other),
    __parent(other.__parent),
    __key(other.__key)
{ }

json::proxy::proxy(json::jobject &parent, const std::string key)
    : json::dynamic_data(parent.get(key)),
    __parent(&parent),
    __key(key)
{
    if(key.length() == 0) throw std::invalid_argument(__FUNCTION__);
}

json::proxy::proxy(json::jobject &parent, const char *key)
    : json::dynamic_data(parent.get(std::string(key))),
    __parent(&parent),
    __key(key)
{
    if(strlen(key) == 0) throw std::invalid_argument(__FUNCTION__);
}

json::proxy& json::proxy::operator=(const json::proxy &other)
{
    if(this == &other) return *this;

    json::dynamic_data::operator=(other);
    this->__parent = other.__parent;
    this->__key = other.__key;
    return *this;
}

json::proxy& json::proxy::operator=(const json::jarray &other)
{
    json::dynamic_data::operator=(other);
    return *this;
}

json::proxy& json::proxy::operator=(const json::jobject &other)
{
    json::dynamic_data::operator=(other);
    return *this;
}

void json::proxy::on_reassignment()
{
    if(this->__parent != NULL) {
        this->__parent->set(this->__key, *this);
    }
}

json::dynamic_data::dynamic_data()
    : json::data_reference()
{ }

json::dynamic_data::dynamic_data(const json::data_reference &other)
    : json::data_reference(other)
{ }

json::dynamic_data::dynamic_data(const uint8_t value)
    : json::data_reference(new integer_data_source<uint8_t>(value))
{ }

json::dynamic_data::dynamic_data(const int8_t value)
    : json::data_reference(new integer_data_source<int8_t>(value))
{ }

json::dynamic_data::dynamic_data(const uint16_t value)
    : json::data_reference(new integer_data_source<uint16_t>(value))
{ }

json::dynamic_data::dynamic_data(const int16_t value)
    : json::data_reference(new integer_data_source<int16_t>(value))
{ }

json::dynamic_data::dynamic_data(const uint32_t value)
    : json::data_reference(new integer_data_source<uint32_t>(value))
{ }

json::dynamic_data::dynamic_data(const int32_t value)
    : json::data_reference(new integer_data_source<int32_t>(value))
{ }

json::dynamic_data::dynamic_data(const uint64_t value)
    : json::data_reference(new integer_data_source<uint64_t>(value))
{ }

json::dynamic_data::dynamic_data(const int64_t value)
    : json::data_reference(new integer_data_source<int64_t>(value))
{ }

json::dynamic_data::dynamic_data(const float value)
    : json::data_reference(new floating_point_data_source<float>(value, FLOAT_FORMAT))
{ }

json::dynamic_data::dynamic_data(const double value)
    : json::data_reference(new floating_point_data_source<double>(value, DOUBLE_FORMAT))
{ }

json::dynamic_data::dynamic_data(const std::string &value)
    : json::data_reference(new string_data_source(value, false))
{ }

json::dynamic_data::dynamic_data(const json::jarray &value)
    : json::data_reference(new jarray_data_source(value))
{ }

json::dynamic_data::dynamic_data(const json::jobject &value)
    : json::data_reference(new jobject_data_source(value))
{ }

json::dynamic_data::dynamic_data(const bool value)
    : json::data_reference(new bool_data_source(value))
{ }

json::dynamic_data& json::dynamic_data::operator=(const json::data_reference &other)
{
    json::data_reference::operator=(other);
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const uint8_t value)
{
    this->reassign(new integer_data_source<uint8_t>(value));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const int8_t value)
{
    this->reassign(new integer_data_source<int8_t>(value));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const uint16_t value)
{
    this->reassign(new integer_data_source<uint16_t>(value));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const int16_t value)
{
    this->reassign(new integer_data_source<int16_t>(value));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const uint32_t value)
{
    this->reassign(new integer_data_source<uint32_t>(value));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const int32_t value)
{
    this->reassign(new integer_data_source<int32_t>(value));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const uint64_t value)
{
    this->reassign(new integer_data_source<uint64_t>(value));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const int64_t value)
{
    this->reassign(new integer_data_source<int64_t>(value));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const float value)
{
    this->reassign(new floating_point_data_source<float>(value, FLOAT_FORMAT));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const double value)
{
    this->reassign(new floating_point_data_source<double>(value, DOUBLE_FORMAT));
    return *this;
}

void json::dynamic_data::set_string(const std::string &value)
{
    this->reassign(new string_data_source(value, false));
}

json::dynamic_data& json::dynamic_data::operator=(const std::string &value)
{
    this->reassign(new string_data_source(value, false));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const char * value)
{
    this->set_string(std::string(value));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const json::jarray &value)
{
    this->reassign(new jarray_data_source(value));
    return *this;
}

json::dynamic_data& json::dynamic_data::operator=(const json::jobject &value)
{
    this->reassign(new jobject_data_source(value));
    return *this;
}

void json::dynamic_data::set_true()
{
    this->reassign(new bool_data_source(true));
}

void json::dynamic_data::set_false()
{
    this->reassign(new bool_data_source(false));
}

void json::dynamic_data::set_null()
{
    this->reassign(new null_data_source());
}

json::data_reference json::data_reference::create(json::data_source * source)
{
    return json::data_reference(source);
}

json::data_reference::data_reference()
    : json::data_source(),
    __source(new null_data_source()),
    __refs(new size_t(1))
{ }

json::data_reference::data_reference(json::data_source *source)
    : __source(source),
    __refs(new size_t(1))
{ }

json::data_reference::data_reference(const json::data_reference &other)
    : __source(other.__source),
    __refs(other.__refs)
{
    assert(*this->__refs > 0);
    (*this->__refs)++;
}

json::data_reference::~data_reference()
{
    this->detatch();
}

json::data_reference& json::data_reference::operator=(const json::data_reference &other)
{
    if(this == &other && this->__source == other.__source) return *this;

    this->detatch();
    this->__source = other.__source;
    this->__refs = other.__refs;
    assert(*this->__refs > 0);
    (*this->__refs)++;
    this->on_reassignment();
    return *this;
}

void json::data_reference::reassign(json::data_source * source)
{
    this->detatch();
    this->__source = source;
    this->__refs = new size_t(1);
    this->on_reassignment();
}

void json::data_reference::detatch()
{
    if(*this->__refs == 1) {
        delete this->__source;
        delete this->__refs;
        this->__source = NULL;
        this->__refs = NULL;
    } else {
        (*this->__refs)--;
    }
}

json::jobject::istream::istream()
    : __state(INTIALIZED),
    __bytes_accepted(0)
{ }

json::jistream::push_result json::jobject::istream::push(const char next)
{
    switch (this->__state)
    {
    case INTIALIZED:
        if(std::isspace(next)) return json::jistream::WHITESPACE;
        if(next == '{') {
            this->__bytes_accepted = 1;
            this->on_object_opened();
            this->__state = AWAITING_NEXT;
            return json::jistream::ACCEPTED;
        }
        return json::jistream::REJECTED;
        break;
    case OPENED:
        if(next == '}') {
            this->__bytes_accepted++;
            this->__state = CLOSED;
            return json::jistream::ACCEPTED;
        }
        // Fall-through
    case AWAITING_NEXT:
        if(std::isspace(next)) return WHITESPACE;
        if(next == '"') {
            this->__bytes_accepted++;
            this->__value = new string_parser();
            if(this->__value->push(next) != ACCEPTED) throw std::logic_error("Unexpected state");
            this->__state = READING_KEY;
            return ACCEPTED;
        }
        return REJECTED;
    case READING_KEY:
        assert(this->__value->type() == json::jtype::jstring);
        switch (this->__value->push(next))
        {
        case ACCEPTED:
            this->__bytes_accepted++;
            if(this->__value->is_valid()) {
                this->__key = this->__value->emit().as_string();
                delete this->__value;
                this->__value = NULL;
                this->__state = AWAITING_COLON;
            }
            return ACCEPTED;
            break;
        case REJECTED:
            return REJECTED;
            break;
        default:
            throw std::logic_error("Unexpected return");
            break;
        }
    case AWAITING_COLON:
        if(std::isspace(next)) return WHITESPACE;
        if(next == ':') {
            this->__bytes_accepted++;
            this->__state = AWAITING_VALUE;
            return ACCEPTED;
        }
        return REJECTED;
    case AWAITING_VALUE:
        if(std::isspace(next)) return WHITESPACE;
        assert(this->__value == NULL);
        this->__value = create_parser(json::jtype::peek(next));
        if(this->__value == NULL) return REJECTED;

        switch (this->__value->push(next))
        {
        case ACCEPTED:
            this->__bytes_accepted++;
            this->on_key_read(this->__key, json::jtype::peek(next));
            this->__state = READING_VALUE;
            return ACCEPTED;
            break;
        case REJECTED:
            return REJECTED;
        default:
            throw std::logic_error("Unexpected return");
            break;
        }
        break;
    case READING_VALUE:
        switch (this->__value->push(next))
        {
        case ACCEPTED:
            this->__bytes_accepted++;
            return ACCEPTED;
            break;
        case WHITESPACE:
            return WHITESPACE;
            break;
        case REJECTED:
            if(!this->__value->is_valid()) return REJECTED;
            this->on_value_read(this->__key, this->__value->emit());
            this->__key.clear();
            delete this->__value;
            this->__value = NULL;
            break;
            // Fall-through
        }
        // Fall-through
    case VALUE_READ:
        if(std::isspace(next)) {
            this->__state = VALUE_READ;
            return WHITESPACE;
        } else if(next == ',') {
            this->__bytes_accepted++;
            this->__state = AWAITING_NEXT;
            return ACCEPTED;
        } else if(next == '}') {
            this->__bytes_accepted++;
            this->on_object_closed();
            this->__state = CLOSED;
            return ACCEPTED;
        }
        return REJECTED;
        break;
    case CLOSED:
        return REJECTED;
    }
}

bool json::jobject::istream::is_valid() const
{
    return this->__state == CLOSED;
}

void json::jobject::istream::reset()
{
    this->__key.clear();
    if(this->__value != NULL) {
        delete this->__value;
        this->__value = NULL;
    }
    this->__bytes_accepted = 0;
    this->__state = INTIALIZED;
}

json::jobject::parser::parser()
    : __handler(NULL)
{
    this->reset();
    this->__handler = new jobject_parser(this->__data);
}

json::jobject::parser::~parser()
{
    this->reset();
    delete this->__handler;
}

void json::jobject::parser::reset()
{
    jobject_data_source * source = new jobject_data_source();
    this->__source = source;
    this->__data = &source->data;
    this->__ref = json::data_reference::create(this->__source);
    if(this->__handler != NULL)
        this->__handler->reset();
}

const json::jobject& json::jobject::parser::result() const
{
    if(!this->is_valid()) throw std::bad_cast();
    assert(this->__data != NULL);
    return *this->__data;
}

json::data_reference json::jobject::parser::emit() const
{
    if(!this->is_valid()) throw std::bad_cast();
    return this->__ref;
}
