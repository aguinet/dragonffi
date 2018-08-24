from enum import Enum
import pydffi
from purectypes.types import BasicTy,ArrayTy,StructTy,PointerTy,Field,EnumTy,UnionTy,FunctionTy,VoidTy

_Generating = object()

_ty_mapping = {pydffi.StructType: StructTy, pydffi.UnionType: UnionTy, pydffi.ArrayType: ArrayTy}

class GenPureCType:
    def __init__(self):
        self._types = {}
        self._triple = pydffi.native_triple()

    def __call__(self, Ty):
        if isinstance(Ty, pydffi.QualType):
            return self(Ty.type)
        ret = self._types.get(Ty, None)
        if not ret is None:
            return ret

        # This would allow an early termination of the recursion for recursive
        # types.
        if isinstance(Ty, (pydffi.StructType, pydffi.UnionType, pydffi.ArrayType)):
            ret = _ty_mapping[type(Ty)]()
            self._types[Ty] = ret

        try:
            name = next(Ty.names)
        except StopIteration: name = None

        if Ty is None:
            # void
            name = "VoidTy"
            ret = VoidTy()
            self._types[None] = ret
            return ret
        if isinstance(Ty, pydffi.BasicType):
            ret = BasicTy(Ty.portable_format)
            name = "__" + str(Ty.kind).split(".")[1] + "Ty"
        elif isinstance(Ty, pydffi.StructType):
            fields = {}
            for f in Ty:
                fields[f.name] = Field(offset=f.offset, type_=self(f.type))
            ret._fields = fields
        elif isinstance(Ty, pydffi.ArrayType):
            ret._elt_type = self(Ty.elementType())
            ret._elt_count = Ty.count()
        elif isinstance(Ty, pydffi.PointerType):
            ret = PointerTy(self(Ty.pointee()), Ty.portable_format)
        elif isinstance(Ty, pydffi.EnumType):
            ret = EnumTy(pydffi.portable_format(Ty), Enum(name, dict(iter(Ty))))
        elif isinstance(Ty, pydffi.UnionType):
            ret._types = {t.name: self(t.type) for t in Ty}
        elif isinstance(Ty, pydffi.FunctionType):
            ret = FunctionTy(self(Ty.returnType), tuple(self(a) for a in Ty.params), Ty.varArgs)
        else:
            raise ValueError("unsupported type %s" % repr(Ty))
        ret._triple = self._triple
        ret._size = Ty.size
        ret._align = Ty.align
        ret._name = name
        self._types[Ty] = ret
        return ret
