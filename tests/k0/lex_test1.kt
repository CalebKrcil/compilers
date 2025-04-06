#!/usr/bin/env kotlin

fun main() {
    // comment
    /*
        multiline followed by whitespace and newline
    */
    


    val a: Int = 10;
    val b: Float = 10.10;
    val d: String = "wow"
    val e: Int = -10
    val f: Float = -10.10
    val g: Boolean = true
    val h: Boolean = false
    val i: String = """
    string
    """

    for (i in 1..5) {
        if (i == 3) {
            break
        }
        println(i)
    }

    for (i in 1..5) {
        if (i == 3) {
            continue
        }
        println(i)
    }

    do {
        println(i)
        i++
    } while (i < 3)

    val condition: Boolean = true
    if (condition) {
        println("True")
    } else {
        println("False")
    }

    for (i in 1..3) {
        println(i)
    }

    for (i in 1..<5) {
        println(i)
    }

    const val CONSTANT_VALUE: Int = 100
    println("Constant: $CONSTANT_VALUE")

    // Demonstrating variable declarations
    val constantValue: Int = 10
    var mutableVariable: Int = 5

    // Arithmetic operators
    val addition: Int = constantValue + mutableVariable
    val subtraction: Int = constantValue - mutableVariable
    val multiplication: Float = constantValue * mutableVariable
    val division: Float = constantValue / mutableVariable
    val modulus: Float = constantValue % mutableVariable

    // Increment and decrement
    mutableVariable++
    mutableVariable--

    // Comparison operators
    val isLessThan: Boolean = constantValue < mutableVariable
    val isGreaterThan: Boolean = constantValue > mutableVariable
    val isLessOrEqual: Boolean = constantValue <= mutableVariable
    val isGreaterOrEqual: Boolean = constantValue >= mutableVariable

    // Equality operators
    val isEqual: Boolean = (constantValue == mutableVariable)
    val isStrictEqual: Boolean = (constantValue === mutableVariable)
    val isNotEqual: Boolean = (constantValue != mutableVariable)

    // Logical operators
    val trueValue: Boolean = true
    val falseValue: Boolean = false
    val conjunction: Boolean = trueValue && falseValue
    val disjunction: Boolean = trueValue || falseValue
    val negation: Boolean = !trueValue
    val negation2: Boolean = ! trueValue

    // Control flow
    if (constantValue > 0) {
        println("Positive value")
    } else {
        println("Non-positive value")
    }

    // Nullable and safe call
    val nullableValue: String? = null

    // Import and const (demonstrating these keywords)
    println("Lexer symbol demonstration complete!")

    var num1: Int = 1
    var num2: Int = 2
    var num3: Int = 3
    var functioncalled: Int = extra(num1, num2, num3)

    val text: String = "Hello"
    val length: Int = text.length
}

fun extra(num1: Int, num2: Int, num3: Int) {
    num1 += 4
    num2 -= 1
    var result: Int = num1 + num2 + num3
    return result
}