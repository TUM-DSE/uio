from typing import List, Any, Iterator, Dict, DefaultDict, Optional, TypeVar

U = TypeVar('U')
def unwrap(a: Optional[U]) -> U: 
    assert a is not None
    return a


def unsafe_cast(a: Any) -> Any:
    """
    this function does nothing but statisfy the type system
    """
    return a



