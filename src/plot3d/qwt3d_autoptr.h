#ifndef QWT3D_AUTOPTR_H
#define QWT3D_AUTOPTR_H



/**
 * @brief Simple auto pointer providing deep copies for raw pointer
 * @details Requirements:\n
 *          virtual T* T::clone() const;\n
 *          T::destroy() const;\n
 *          virtual ~T() private/protected\n\n
 *          clone() is necessary for the pointer to preserve polymorphic behaviour.
 *          The pointer requires also heap based objects with regard to the template
 *          argument in order to be able to get ownership and control over destruction.
 */
template< typename T >
class Qwt3DClonePtr
{
public:
    // Standard ctor
    explicit Qwt3DClonePtr(T* ptr = nullptr) : rawptr_(ptr)
    {
    }
    // Dtor (calls T::destroy)
    ~Qwt3DClonePtr()
    {
        destroyRawPtr();
    }

    // Copy ctor (calls (virtual) clone())
    Qwt3DClonePtr(Qwt3DClonePtr const& val)
    {
        rawptr_ = val.rawptr_->clone();
    }

    // Assignment in the same spirit as copy ctor
    Qwt3DClonePtr< T >& operator=(Qwt3DClonePtr const& val)
    {
        if (this == &val)
            return *this;

        destroyRawPtr();
        rawptr_ = val.rawptr_->clone();

        return *this;
    }

    // Pointer-like access operator
    T* operator->() const
    {
        return rawptr_;
    }

    // Dereferencing operator
    T& operator*() const
    {
        return *rawptr_;
    }

private:
    T* rawptr_;
    void destroyRawPtr()
    {
        if (rawptr_)
            rawptr_->destroy();
        rawptr_ = nullptr;
    }
};


#endif  // QWT3D_AUTOPTR_H
