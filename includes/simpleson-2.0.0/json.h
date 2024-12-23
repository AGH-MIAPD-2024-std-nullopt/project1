#ifndef JSON_H
#define JSON_H

/*! \file json.h
 * \brief Simpleson header file
 */

#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <cstdio>
#include <utility>
#include <typeinfo>
#include <stdexcept>
#include <cctype>
#include <stdint.h>

/*! \brief Base namespace for simpleson */
namespace json
{
	// Forward declaration
	class jobject;

	// Forward declaration
	class jarray;

	/*! \brief Exception used when invalid JSON is encountered */
	class parsing_error : public std::invalid_argument
	{
	public:
		/*! \brief Constructor 
		 *
		 * @param message Details regarding the parsing error
		 */
		inline parsing_error(const char *message) : std::invalid_argument(message) { }

		/*! \brief Constructor 
		 *
		 * @param message Details regarding the parsing error
		 */
		inline parsing_error(const std::string &message) : std::invalid_argument(message) { }

		/*! \brief Destructor */
		inline virtual ~parsing_error() throw() { }
	};

	/*! \brief Exception used when an unexpected charater is encountered */
	class unexpected_character : public parsing_error
	{
	public:
		/*! \brief Constructor
		 *
		 * @param value The character that was not expected in the stream
		 */
		inline unexpected_character(const char value)
			: parsing_error("Unexpected character '" + std::string(value, 1) + "'"),
			rejected_char(value)
		{ }

		/*! \brief The character that was not expected in the stream */
		const char rejected_char;
	};

	/*\brief Alias for a list of keys */
	typedef std::vector<std::string> key_list_t;

	/* \brief Namespace for handling of JSON data types */
	namespace jtype
	{
		/*! \brief Descriptor for the type of JSON data */
		enum jtype { 
			jstring, ///< String value
			jnumber, ///< Number value
			jobject, ///< JSON object
			jarray, ///< JSON array
			jbool, ///< Boolean value
			jnull, ///< Null value
			not_valid ///< Value does not conform to JSON standard
		};

		/*! \brief Predicts the type of JSON data based on the first character 
		 *
		 * @param input The first character of the serialized data
		 * \returns The predicted type of JSON data
		 */
		jtype peek(const char input);
	}

	/*! \brief Interface for sources of JSON data */
	class data_source
	{
	public:
		data_source();
		virtual ~data_source();
		virtual jtype::jtype type() const = 0;
		virtual std::string serialize() const = 0;

		virtual inline operator uint8_t() const { throw std::bad_cast(); }
		virtual inline operator int8_t() const { throw std::bad_cast(); }
		virtual inline operator uint16_t() const { throw std::bad_cast(); }
		virtual inline operator int16_t() const { throw std::bad_cast(); }
		virtual inline operator uint32_t() const { throw std::bad_cast(); }
		virtual inline operator int32_t() const { throw std::bad_cast(); }
		virtual inline operator uint64_t() const { throw std::bad_cast(); }
		virtual inline operator int64_t() const { throw std::bad_cast(); }
		virtual inline operator float() const { throw std::bad_cast(); }
		virtual inline operator double() const { throw std::bad_cast(); }
		virtual operator jarray() const; // Inline definition not possible due to forward declaration
		virtual operator jobject() const; // Inline definition not possible due to forward declaration

		jarray as_array() const;
		jobject as_object() const;

		/*! \brief Tempalte method for casting the data source to another data type */
		template<typename T>
		T cast() const { return this->operator T; }

		virtual std::string as_string() const = 0;
		virtual bool as_bool() const { throw std::bad_cast(); }

		inline bool is_string() const { return this->type() == jtype::jstring; }
		inline bool is_number() const { return this->type() == jtype::jnumber; }
		inline bool is_object() const { return this->type() == jtype::jobject; }
		inline bool is_array() const { return this->type() == jtype::jarray; }
		inline bool is_bool() const { return this->type() == jtype::jbool; }
		inline bool is_null() const { return this->type() == jtype::jnull; }
	};

	/*! \brief Reference to a data source
	 *
	 * Data references are used as a pointer to a data source. 
	 * 
	 */
	class data_reference : virtual public data_source
	{
	public:
		/*! \brief Creates a data reference from a data source 
		 *
		 * @param input A pointer to the data source
		 * \returns A data reference to the data source
		 */
		static data_reference create(data_source * input);

		/*! \brief Default constructor */
		data_reference();

		/*! \brief Copy constructor */
		data_reference(const data_reference &other);

		/*! \brief Destructor */
		virtual ~data_reference();

		/*! \brief Assignment operator */
		data_reference& operator=(const data_reference &other);

		/*! \brief Reference to the data source 
		 *
		 * \returns A reference to the underlying data_source
		 */
		const data_source & ref() const { return *this->__source; }

		virtual inline jtype::jtype type() const { return this->__source->type(); }
		virtual inline std::string serialize() const { return this->__source->serialize(); }

		virtual inline operator uint8_t() const { return this->__source->operator uint8_t(); }
		virtual inline operator int8_t() const { return this->__source->operator int8_t(); }
		virtual inline operator uint16_t() const { return this->__source->operator uint16_t(); }
		virtual inline operator int16_t() const { return this->__source->operator int16_t(); }
		virtual inline operator uint32_t() const { return this->__source->operator uint32_t(); }
		virtual inline operator int32_t() const { return this->__source->operator int32_t(); }
		virtual inline operator uint64_t() const { return this->__source->operator uint64_t(); }
		virtual inline operator int64_t() const { return this->__source->operator int64_t(); }
		virtual inline operator float() const { return this->__source->operator float(); }
		virtual inline operator double() const { return this->__source->operator double(); }
		virtual operator jarray() const; // Inline definition not possible due to forward declaration
		virtual operator jobject() const; // Inline definition not possible due to forward declaration
		virtual inline std::string as_string() const { return this->__source->as_string(); }
		virtual inline bool as_bool() const { return this->__source->as_bool(); }
	protected:
		/*! \brief Constructs a data referernce from a source
		 *
		 * \note This constructor must be kept private when data_referernce derives from \ref data_source, otherwise there is ambiguity between the copy constructor and this constructor. 
		 */
		data_reference(data_source * source);

		/*! \brief Reassigns the data source
		 */
		void reassign(data_source * source);

		/*! \brief Callback function for reassignment */
		virtual inline void on_reassignment() { }
	private:
		void detatch();
		data_source * __source;
		size_t * __refs;
	};

	/*! \brief Container for JSON-encoded data 
	 *
	 * This class can hold any type of JSON-encoded data. The type of data it can hold can be re-assigned dynamically as well. 
	 */
	class dynamic_data : public data_reference
	{
	public:
		dynamic_data();
		dynamic_data(const data_reference &other);
		dynamic_data(const uint8_t value);
		dynamic_data(const int8_t value);
		dynamic_data(const uint16_t value);
		dynamic_data(const int16_t value);
		dynamic_data(const uint32_t value);
		dynamic_data(const int32_t value);
		dynamic_data(const uint64_t value);
		dynamic_data(const int64_t value);
		dynamic_data(const float value);
		dynamic_data(const double value);
		dynamic_data(const std::string &value);
		dynamic_data(const json::jarray &value);
		dynamic_data(const json::jobject &value);
		dynamic_data(const bool value);

		dynamic_data& operator=(const data_reference &other);

		dynamic_data& operator=(const uint8_t value);
		dynamic_data& operator=(const int8_t value);
		dynamic_data& operator=(const uint16_t value);
		dynamic_data& operator=(const int16_t value);
		dynamic_data& operator=(const uint32_t value);
		dynamic_data& operator=(const int32_t value);
		dynamic_data& operator=(const uint64_t value);
		dynamic_data& operator=(const int64_t value);
		dynamic_data& operator=(const float value);
		dynamic_data& operator=(const double value);
		dynamic_data& operator=(const jarray &value);
		dynamic_data& operator=(const jobject &value);
		void set_string(const std::string &value);
		dynamic_data& operator=(const std::string &value);
		dynamic_data& operator=(const char * value);
		void set_true();
		void set_false();
		void set_null();
	};

	/*! \brief Interface for streaming input data */
	class jistream
	{
	public:
		/*! \brief Default constructor */
		jistream();
		
		/*! \brief Destructor */
		virtual ~jistream();

		/*! \brief Enumeration for the result of a push to the stream*/
		enum push_result
		{
			ACCEPTED, ///< The character was valid. Reading should continue. 
			REJECTED, ///< The character was not valid. Reading should stop.
			WHITESPACE ///< The character was whitespace. Reading should continue but the whitespace was not stored. 
		};

		/*! \brief Checks the type of JSON data being fed to the stream */
		virtual jtype::jtype type() const = 0;

		/*!\ brief Pushes a value to the back of the reader 
		*
		* @param next the value to be pushed
		* \returns `ACCEPTED` if the value was added to the reader, `WHITESPACE` if the input was whitespace that was not stored, and `REJECTED` is the input was invalid for the value type
		*/
		virtual push_result push(const char next) = 0;

		/*! \brief Operator for streaming data
		 * 
		 * \returns `false` if the push was successful, `true` if the pust was rejected. 
		 * \note Whitespace is considered a successful push. \see push()
		 */
		inline bool operator<< (const char next)
		{
			return this->push(next) != REJECTED;
		}

		virtual bool is_valid() const = 0;
		virtual void reset() = 0;

		/*! \brief Pushes a buffer to the stream 
		 *
		 * \param buffer The buffer to push into the stream
		 * \param buf_len The length of the buffer
		 * \returns The number of bytes pushed. If all bytes are accepted or whitespace, then the method should return the same value as buf_len. If a value is rejected, then the method will return the number of bytes successfully pushed. 
		 */
		inline size_t read(const char *buffer, const size_t buf_len)
		{
			for(size_t i = 0; i < buf_len; i++)
				if(this->push(buffer[i]) == REJECTED) return i;
			return buf_len;
		}

		/*! \brief Pushes a string to the stream */
		inline size_t read(const std::string &input)
		{
			return this->read(input.c_str(), input.length());
		}
	};

	/*! \brief Class for parsing JSON data via a stream */
	class data_parser : virtual public jistream
	{
	public:
		/*! \brief Default constructor */
		data_parser();

		/*! \brief Destructor */
		virtual ~data_parser();

		/*! \brief Translate the stream into JSON data */
		virtual data_reference emit() const = 0;
	};

	/*! \brief Class for handling JSON arrays */
	class jarray : public std::vector<json::dynamic_data>
	{
	public:
		/*! \brief Default constructor */
		inline jarray() : std::vector<json::dynamic_data>() { }

		inline jtype::jtype type() const { return jtype::jarray; }

		template<typename T>
		jarray& operator=(const std::vector<T> input)
		{
			this->set(input);
			return *this;
		}

		template<typename T>
		jarray(const std::vector<T> input)
		{
			this->set(input);
		}

		template<typename T>
		void set(const std::vector<T> input)
		{
			this->clear();
			this->reserve(input.size());
			for(size_t i = 0; i < input.size(); i++)
			{
				this->push_back((T)input.at(i));
			}
		}

		/*! \brief Parses a null-terminated JSON array */
		static jarray parse(const char *input);

		/*! \brief Parses a JSON array */
		static inline jarray parse(const std::string &input) { return jarray::parse(input.c_str()); }

		/*! \brief Serializes the JSON array into a string */
		std::string as_string() const;

		/*! \brief Alias of ::as_string() */
		virtual inline std::string serialize() const { return this->as_string(); }

		/*! \brief Casts the JSON array as a string 
		 * \see ::as_string()
		 */
		inline operator std::string() const { return this->as_string(); }

		/*! \brief Converts the JSON array into a formated string 
		 *
		 * \param indent_level The starting number of indents to use
		 */
		std::string pretty(unsigned int indent_level = 0) const;

		/*! \brief Casts the JSON array into a vector of strings */
		inline operator std::vector<std::string>() const
		{
			std::vector<std::string> result;
			for(size_t i = 0; i < this->size(); i++)
			{
				result.push_back(this->at(i).as_string());
			}
			return result;
		}

		/*! \brief Tempalate for casting the JSON array into a vector of values */
		template<typename T>
		operator std::vector<T>() const
		{
			std::vector<T> result;
			for(size_t i = 0; i < this->size(); i++)
			{
				result.push_back((T)this->at(i));
			}
			return result;
		}

		/*! \brief Class for streaming JSON arrays 
		 *
		 * This class is useful for applications where it is not desirable to load the entire JSON array in memory. 
		 * An example use case is an embedded application that does not have sufficent memory to parse a large JSON array. 
		 * In this case, the istream can "listen" for data and action the data when it is parsed without having to load the entire JSON array. 
		 * 
		 * \example jarray_istream.cpp
	 	 * This is a basic of example of using a JSON array stream
		 */
		class istream : virtual public jistream
		{
		public:
			/*! \brief Default constructor */
			istream();

			/*! \brief Destructor */
			virtual inline ~istream() { }

			virtual inline jtype::jtype type() const { return json::jtype::jarray; }
			virtual push_result push(const char next);
			virtual bool is_valid() const;
			virtual void reset();
			inline size_t bytes_accepted() const { return this->__bytes_accepted; }
		protected:
			/*! \brief Callback for when an array is opened */
			virtual void on_array_opened() = 0;

			/*! \brief Callback for value read 
			 *
			 * \note If the size of non-whitespace value exceeded the buffer length, then this method will never be called. \see on_key_read()
			 */
			virtual void on_value_read(const data_reference &value) = 0;

			// virtual void on_value_overflow(const char input) = 0;

			/*! \brief Callback for when the array is closed
			 *
			 * After this callback, calls to is_closed() will return `true` and calls to push() will return `REJECTED` until reset() is called
			 * \note It is safe to call reset() from this method to reset the stream
			 */
			virtual void on_array_closed() = 0;
		private:
			/*! \brief The buffer used to read data
			 *
			 * \note This buffer is used to read the key and then is reset before reading the value. 
			 */
			json::data_parser * __value;

			/*! \brief Enum for tracking the state of the stream */
			enum state
			{
				INTIALIZED, ///< The array has not been opened. Initial state of the stream. 
				OPENED, ///< The array has been opened. Will jump to READING_VALUE or CLOSED. 
				AWAITING_VALUE, ///< Will jump to READING_VALUE when the start of the value is encountered. 
				READING_VALUE, ///< Reading the value. Can jump to AWAITING_NEXT or CLOSED. 
				VALUE_READ, ///< Value read is complete. Waiting for comma or end bracket
				CLOSED ///< Array has been completely read. is_closed() will return `true`.
			};

			/*! \brief The current state of the stream. */
			state __state;

			/*! \brief The number of bytes that have been accepted */
			size_t __bytes_accepted;
		};

		/*! \brief Class for parsing JSON arrays 
		 * 
		 * \example jarray_parser.cpp
	 	 * This is a basic of example of using a JSON arry parser
		 */
		class parser : public data_parser
		{
		public:
			/*! \brief Default constructor */
			parser();

			/*! \brief Destructor */
			virtual ~parser();

			virtual inline jtype::jtype type() const { return json::jtype::jarray; }
			virtual inline push_result push(const char next) { return this->__handler->push(next); }
			virtual inline bool is_valid() const { return this->__handler->is_valid(); }
			virtual void reset();
			const jarray& result() const;
			data_reference emit() const;
		private:
			jarray * __data;
			istream * __handler;
			data_source * __source;
			data_reference __ref;
		};
	};

	/*! \brief Class for representing and manipulating data contained within a JSON object 
	 *
	 * \warning Destructing the underlying \ref jobject befure the proxy is destructed will result in undefined behavior 
	 */
	class proxy : public dynamic_data
	{
	public:
		/*! \brief Default constructor */
		proxy();

		/*! \brief Copy constructor */
		proxy(const proxy &other);

		/*! \brief Creates a proxy
		 * 
		 * \param parent The parent JSON object
		 * \param key The key for the data to be referenced
		 */
		proxy(jobject &parent, const std::string key);

		/*! \brief Creates a proxy
		 * 
		 * \param parent The parent JSON object
		 * \param key The key for the data to be referenced
		 */
		proxy(jobject &parent, const char *key);

		/*! \brief Copy assignment */
		proxy& operator=(const proxy &other);

		proxy& operator=(const jarray &other);
		proxy& operator=(const jobject &other);

		/*! \brief Assigns a new value 
		 *
		 * \note This method will manipulate the parent JSON object
		 */
		template<typename T>
		proxy& operator=(const T value) { dynamic_data::operator=(value); return *this;}
	protected:
		jobject *__parent;
		std::string __key;
		virtual void on_reassignment();
	};

	typedef std::map<std::string, json::data_reference> jmap;

	/*! \class jobject
	 * \brief The class used for manipulating JSON objects and arrays
	 *
	 * \example jobject.cpp
	 * This is a basic of example of using simpleson for manipulating JSON
	 *
	 * \example rootarray.cpp
	 * This is an example of how to handle JSON where the root object is an array
	 * 
	 * \example objectarray.cpp
	 * This is an example of how to handle an array of JSON objects
	 */
	class jobject : private jmap
	{
	public:
		/*! \brief Default constructor
		 */
		inline jobject() : jmap() { }

		/*! \brief Copy constructor */
		inline jobject(const jobject &other)
			: jmap(other)
		{ }

		virtual inline ~jobject() { }

		virtual inline jtype::jtype type() const { return jtype::jobject; }

		/*! \brief Returns the number of entries in the JSON object or array */
		using jmap::size;

		/*! \brief Clears the JSON object or array */
		using jmap::clear;

		/*! \brief Comparison operator
		 *
		 * \todo Currently, the comparison just seralizes both objects and compares the strings, which is probably not as efficent as it could be
		 */
		bool operator== (const json::jobject other) const { return ((std::string)(*this)) == (std::string)other; }

		/*! \brief Comparison operator */
		bool operator!= (const json::jobject other) const { return ((std::string)(*this)) != (std::string)other; }

		/*! \brief Assignment operator */
		inline jobject& operator=(const jobject &rhs)
		{
			jmap::operator=(rhs);
			return *this;
		}

		using jmap::insert;

		/*! \brief Parses a serialized JSON string
		 *
		 * @param input Serialized JSON string
		 * @return JSON object or array
		 * \exception json::parsing_error Thrown when the input string is not valid JSON
		 */
		static jobject parse(const char *input);

		/*! \brief Parses a serialized JSON string 
		 *
		 * @see json::jobject::parse(const char*)
		 */
		static inline jobject parse(const std::string input) { return parse(input.c_str()); }

		/*! \brief Determines if an object contains a key
		 *
		 * @param key The key to check for
		 * @return True if the object contains the provided key and false if the object does not contain the key
		 * \note If the object represents a JSON array, then this function will always return false
		 */
		inline bool has_key(const std::string &key) const
		{
			return this->find(key) != this->end();
		}

		/*! \brief Returns a list of the object's keys
		 *
		 * @return A list of keys contained in the object. If the object is actionally an array, an empty list will be returned
		 */
		key_list_t list_keys() const;

		/*! \brief Sets the value assocaited with the key
		 *
		 * \details If the key exists, then the value is updated. If the key does not exist, then the key value pair is added to the object. 
		 * @param key The key for the entry
		 * @param value The value for the entry
		 * \exception json::invalid_key Exception thrown if the object actually represents a JSON array
		 */
		void set(const std::string &key, const data_reference &value);

		/*! \brief Returns the serialized value associated with a key
		 * 
		 * @param key The key for the desired element
		 * @return A serialized representation of the value associated with the key
		 * \exception json::invalid_key Exception thrown if the key does not exist in the object or the object actually represents a JSON array
		 */
		inline const json::data_reference get(const std::string &key) const
		{
			jmap::const_iterator it = this->find(key);
			if(it == this->end()) return json::dynamic_data();
			return it->second;
		}

		/*! \brief Removes the entry associated with the key
		 *
		 * @param key The key of the key value pair to be removed
		 * \note If the key is not found in the object, no action is taken
		 */
		void remove(const std::string &key);

		/*! \brief Returns an element of the JSON object
		 * 
		 * @param key The key of the element to be returned
		 * @return A proxy for the value paired with the key
		 * \exception json::invalid_key Exception thrown if the object is actually a JSON array
		 */
		inline proxy operator[](const std::string &key)
		{
			return json::proxy(*this, key);
		}

		/*! \see json::jobject::as_string() */
		operator std::string() const;

		/*! \brief Serialzes the object or array 
		 * \note The serialized object or array will be in the most compact form and will not contain any extra white space, even if the serialized string used to generate the object or array contained extra white space. 
		 */
		inline std::string as_string() const
		{
			return this->operator std::string();
		}

		virtual inline std::string serialize() const { return this->as_string(); }

		/*! \brief Returns a pretty (multi-line indented) serialzed representation of the object or array
		 * 
		 * @param indent_level The number of indents (tabs) to start with
		 * @return A "pretty" version of the serizlied object or array
		 */
		std::string pretty(unsigned int indent_level = 0) const;

		/*! \brief Class for streaming JSON objects 
		 *
		 * This class is useful for applications where it is not desirable to load the entire JSON object in memory. 
		 * An example use case is an embedded application that does not have sufficent memory to parse a large JSON object. 
		 * In this case, the istream can "listen" for certain keys and action the data when it is parsed without having to load the entire JSON object. 
		 * 
		 * \example jobject_istream.cpp
	 	 * This is a basic of example of using a JSON object stream
		 */
		class istream : virtual public jistream
		{
		public:
			virtual inline jtype::jtype type() const { return json::jtype::jobject; }
			istream();
			virtual inline ~istream() { }
			virtual push_result push(const char next);
			virtual bool is_valid() const;
			virtual void reset();
			inline size_t bytes_accepted() const { return this->__bytes_accepted; }
		protected:
			virtual void on_object_opened() = 0;

			/*! \brief Callback for key read
			 * 
			 * \param key They key that has been read
			 * \returns The number of bytes to read before characters are discarded. Every excess character will result in on_char_discarded() to be called. 
			 * 			 
			 * Returning 0 from this function and implementing a custom value reader through on_char_discarded() is potential approach for handling large payloads, such as binary data. 
			 * \note If the value length exceeds the value returned from this method, then on_value_read() will never be called. 
			 * \note Whitespace does not count against the value length. \see push_result
			 */
			virtual void on_key_read(const std::string &key, const json::jtype::jtype type) = 0;

			/*! \brief Callback for value read 
			 *
			 * \note If the size of non-whitespace value exceeded the buffer length, then this method will never be called. \see on_key_read()
			 */
			virtual void on_value_read(const std::string &key, const data_reference &value) = 0;

			// virtual void on_value_overflow(const char input) = 0;

			/*! \brief Callback for when the object is closed
			 *
			 * After this callback, calls to is_closed() will return `true` and calls to push() will return `REJECTED` until reset() is called
			 * \note It is safe to call reset() from this method to reset the stream
			 */
			virtual void on_object_closed() = 0;
		private:
			/*! \brief The key for the current entry */
			std::string __key;

			/*! \brief The buffer used to read data
			 *
			 * \note This buffer is used to read the key and then is reset before reading the value. 
			 */
			json::data_parser * __value;

			/*! \brief Enum for tracking the state of the stream */
			enum state
			{
				INTIALIZED, ///< The object has not been opened. Initial state of the stream. 
				OPENED, ///< The object has been opened. Will jump to READING_KEY or CLOSED. 
				READING_KEY, ///< A key is being read. Will jump to AWAITING_COLON after key read. 
				AWAITING_COLON, ///< A key has been read but the colon has not been encountered. Will jump to AWAITING_VALUE once the colon is encountered. 
				AWAITING_VALUE, ///< A key and colon has been read. Will jump to READING_VALUE when the start of the value is encountered. 
				READING_VALUE, ///< Reading the value. Can jump to AWAITING_NEXT or CLOSED. 
				VALUE_READ, ///< Value read is complete. Waiting for comma or end brace
				AWAITING_NEXT, ///< Waiting for the next key. Will jump to READING_KEY. 
				CLOSED ///< Object has been completely read. is_closed() will return `true`.
			};

			/*! \brief The current state of the stream. */
			state __state;

			/*! \brief The number of bytes that have been accepted */
			size_t __bytes_accepted;
		};

		/*! \brief Class for parsing JSON objects 
		 * \example jobject_parser.cpp
	 	 * This is a basic of example of using a JSON object parser
		 */
		class parser : public data_parser
		{
		public:
			/*! \brief Default constructor */
			parser();

			/*! \brief Destructor */
			virtual ~parser();
			virtual inline jtype::jtype type() const { return json::jtype::jobject; }
			virtual inline push_result push(const char next) { return this->__handler->push(next); }
			virtual inline bool is_valid() const { return this->__handler->is_valid(); }
			virtual void reset();
			const jobject& result() const;
			data_reference emit() const;
		private:
			jobject * __data;
			istream * __handler;
			data_source * __source;
			data_reference __ref;
		};
	};
}

#endif // !JSON_H