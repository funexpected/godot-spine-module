#pragma once

template <typename T>
class SmartPtr {
private:
	T *pData; // Generic pointer to be stored

public:
	SmartPtr(T *pValue = nullptr) : pData(pValue) { }

	void assign(T *pValue) {
		pData = pValue;
	}

	~SmartPtr() {
		if (pData != nullptr)
			memdelete(pData);
		pData = nullptr;
	}

    bool operator==(const T* val) const
    {
        return val == pData;
    }

	SmartPtr<T>& operator=(T* pValue)
	{
		assign(pValue);
		return *this;
	}

    operator bool() const {
        return pData != nullptr;
    }

    operator T *() const {
        return pData;
    }

	T &operator*() {
		return *pData;
	}

	T *operator->() {
		return pData;
	}
};