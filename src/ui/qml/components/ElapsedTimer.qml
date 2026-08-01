import QtQml

QtObject {
    property double _start: 0

    function restart(): void {
        _start = Date.now();
    }

    function elapsed(): real {
        return (Date.now() - _start) / 1000;
    }
}
