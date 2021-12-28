#pragma once

#ifdef _MSC_VER

// Настройки компилятора
//
// Понижение уровня срабатывания предупреждения
// Трактовать предупреждения как ошибки

// Помогает подчищать код
#pragma warning ( 1 : 4189 )// 'identifier' : local variable is initialized but not referenced"

// Спасает от страшных глюков когда функция ничего не вернула, 
// и возвратом оказался мусор в регистре EAX
#pragma warning ( 1 : 4715 )
#pragma warning( error : 4715 )// : not all control paths return a value

// Спасает от креша
#pragma warning( error : 4717 )// : 'function' recursive on all control paths, function will cause runtime stack overflow

// Проверка типа переменных должна работать даже с типом bool
// к сожалению компилер втихаря конвертирует int <- bool и bool <- NULL
#pragma warning ( 1 : 4800 )
#pragma warning( error : 4800 )// 'type' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning ( 1 : 4804 )
#pragma warning( error : 4804 )// 'operation' : unsafe use of type 'bool' in operation


// Спасает от страшных релизодебажных глюков
#pragma warning( error : 4700 )// local variable 'name' used without having been initialized

// Спасает от многих глюков с большими интами.
#pragma warning( error : 4018 )// 'expression' : signed/unsigned mismatch

// Предотвращает крэш
#pragma warning( error : 4541 )// 'identifier' used on polymorphic type 'type' with /GR-; unpredictable behavior may result

// помогает от случайного = вместо ==
// assignment within conditional expression
#pragma warning( 1: 4706 )
#pragma warning( error: 4706 )

// такой класс "class MainDlg : public CDialog, public AVBaseObject" некорректно
// записывает указатели на свои функции в MESSAGE_MAP потому что указатель на 
// функцию-член с виртуальным базовым классом, не такой как просто на функцию-член
// а RefCounter это virtual CRefCounter. Лечиться замной RefCounter->CRefCounter
// cast between different pointer to member representations, compiler may generate incorrect code
#pragma warning( 1: 4407 )
#pragma warning( error: 4407 )

#endif// _MSC_VER
