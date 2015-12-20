#ifndef OBJECTCOMPAT_H
#define OBJECTCOMPAT_H

#include <QHash>
#include <QMetaType>

#include <virtualmetaclass.h>

// FIXME heavy header used only for Quassel::Features; the enums should probably be refactored out
#include "quassel.h"

/**
 * Default downgrade: FallbackType provides a constructor taking
 *  const ModelType &inst, or compatible.
 * Template may be specialized (or other downgrade mechanisms provided)
 *  as needed
 */
template<typename ModelType, typename FallbackType>
inline FallbackType protocolDowngrade(const ModelType *inst) {
    return FallbackType(*inst);
}

template<typename F> struct TypesOf;
template<typename R, typename I>
struct TypesOf< R(I) > { typedef R ReturnType; typedef I InputType; };

template<typename T>
    struct PlainType
    { typedef T Type; };
template<typename T>
    struct PlainType<T const>
    { typedef T Type; };
template<typename T>
    struct PlainType<T const &>
    { typedef T Type; };


class ObjectCompat;
extern ObjectCompat *objectCompat;

/**
 * Central registry for object compatability mechanisms
 */
class ObjectCompat {

public:
    ObjectCompat() : _objectCompatStrategies() {}

    /**
     * Register a type downgrade mechanisms for peers using older protocols.
     *
     * @typename I The newer type, which may need to be downgraded if
     *  an older peer is connected.
     * @typename R The older type, which older peers understand.
     *  Compatability may be chained, so type R may, in another context,
     *  be a separately registered I type with its own R type for
     *  even older clients.
     *
     * @param featureFlag The protocol version where I was added.
     * @param F The function used to convert I to R
     *  FallbackType, when needed by older peers.
     */
    template< class I, class R >
    struct DowngraderRegistreeFactory {
        template<R (*F)(const I *)>
        struct create {
            static inline void forVersionsWithout(Quassel::Feature featureFlag) {
                int (*innerVirtualMetaType)(const void *) = &makeInnerVirtualMetaTypeOf<I>;
                objectCompat->_objectCompatStrategies.insert(
                            qMetaTypeId<typename PlainType<I>::Type>(),
                                  ObjectCompatEntry(
                                      innerVirtualMetaType,
                                      featureFlag,
                                      innerDowngrade));
            }
        private:
            /*
             * We ensure type safety externally.
             * However, static type information is lost once objects are
             *  converted to void *, which the signal system does internally.
             * Within our encapsulation here, we must rely on qMetaTypeId
             *  consistency to keep us safe.
             */
            static QVariant innerDowngrade(Quassel::Features peerProtocolVersion, const void *input) {
                R ret = F((const I *) input);
                // Have we gone back far enough to be compatible with this peer yet?

                // TODO: We know the actual type now. We should be able to
                // resolve other functions needed at compile time, or omit
                // them entirely if we're already at the base case. Using
                // one lookup plus one function pointer call is generally
                // enough.
                auto s = objectCompat->_objectCompatStrategies.find(qMetaTypeId<R>());
                if (s == objectCompat->_objectCompatStrategies.end() ||
                        !s->featureFlag || s->featureFlag & peerProtocolVersion) {
                    return QVariant(qMetaTypeId<R>(), &ret); // QVariant always makes a copy
                }
                return s->downgrade(peerProtocolVersion, &ret);
            }
        };
    };

    template<typename ModelType>
    inline void registerVirtualType() {
        _objectCompatStrategies.insert(
                    qMetaTypeId<ModelType>(),
                    ObjectCompatEntry(
                        makeInnerVirtualMetaTypeOf<ModelType>,
                        Quassel::Feature(0),
                        NULL));
    }

    /**
     * Determine the flags a client needs to have set to understand a type
     * @param typeId
     * @return The flags needed to understand the type
     */
    inline Quassel::Features flagsNeeded(int typeId) {
        auto it = _objectCompatStrategies.find(typeId);
        if (it == _objectCompatStrategies.end())
            return 0;
        return it->featureFlag;
    }

    /**
     * Ensures a model is compatible for a given peer version.
     * @param peerVersion Flags the peer supports
     * @param metaTypeId Declared metatype
     * @param input Actual parameter
     * @return Equivalent parameter which the peer can understand
     */
    inline QVariant peerCompatibleQVariant(Quassel::Features peerVersion, int metaTypeId, void *input) {
        // Find the strategy for this metaTypeId
        auto s = _objectCompatStrategies.find(metaTypeId);
        // Do we not need a special strategy for this model type?
        if (s == _objectCompatStrategies.end()) {
            return QVariant(metaTypeId, input);
        }
        int instanceMetaTypeId = s->virtualMetaTypeOf(input);
        if (instanceMetaTypeId != metaTypeId) {
            // We have a subtype above the declared type, move up
            metaTypeId = instanceMetaTypeId;
            s = _objectCompatStrategies.find(metaTypeId);
            // Does our real type not need a special strategy?
            if (s == _objectCompatStrategies.end()) {
                return QVariant(metaTypeId, input);
            }
        }
        // Is our peer recent enough for us to send the most up to date type?
        if (!s->featureFlag || s->featureFlag & peerVersion) {
            return QVariant(metaTypeId, input);
        }
        // Our peer isn't new enough. We must downgrade the model for them.
        return s->downgrade(peerVersion, input);
    }

    inline bool typesCompatible(int expected, int actual) {
        if (expected == actual) return true;
        if (_objectCompatStrategies.contains(expected)) return true; // FIXME: Should do proper checking
        return false;
    }

private:
    template<typename VariantType>
    static int makeInnerVirtualMetaTypeOf(const void *input) {
        return VirtualMetaClass::virtualMetaTypeOf<VariantType>(input);
    }

    struct ObjectCompatEntry {
        int (*virtualMetaTypeOf)(const void *i);
        Quassel::Feature featureFlag;
        QVariant (*downgrade)(Quassel::Features peerVersion, const void *input);
        ObjectCompatEntry(
                int (*t)(const void *i),
                Quassel::Feature v,
                QVariant (*d)(Quassel::Features peerVersion, const void *input))
            : virtualMetaTypeOf(t), featureFlag(v), downgrade(d) {}
    };
    // metaTypeId -> CompatEntry
    QHash<int, struct ObjectCompatEntry> _objectCompatStrategies;

};

#endif // OBJECTCOMPAT_H
