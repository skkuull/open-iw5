#include <std_include.hpp>
#include "variable_value.hpp"

namespace game::scripting
{
	variable_value::variable_value(const native::VariableValue& value)
	{
		this->assign(value);
	}

	variable_value::variable_value(const variable_value& other) noexcept
	{
		this->operator=(other);
	}

	variable_value::variable_value(variable_value&& other) noexcept
	{
		this->operator=(std::move(other));
	}

	variable_value& variable_value::operator=(const variable_value& other) noexcept
	{
		if (this != &other)
		{
			this->release();
			this->assign(other.value_);
		}

		return *this;
	}

	variable_value& variable_value::operator=(variable_value&& other) noexcept
	{
		if (this != &other)
		{
			this->release();
			this->value_ = other.value_;
			other.value_.type = native::SCRIPT_NONE;
		}

		return *this;
	}

	variable_value::~variable_value()
	{
		this->release();
	}

	const native::VariableValue& variable_value::get() const
	{
		return this->value_;
	}

	void variable_value::assign(const native::VariableValue& value)
	{
		this->value_ = value;
		native::AddRefToValue(&this->value_);
	}

	void variable_value::release()
	{
		if (this->value_.type != native::SCRIPT_NONE)
		{
			native::RemoveRefToValue(this->value_.type, this->value_.u);
			this->value_.type = native::SCRIPT_NONE;
		}
	}
}
