val globalVal: Int = 42
var globalVar: Int = 100

fun noReturn() {
    val message: String = "No return"
    println(message)
}

fun add(x: Int, y: Int): Int {
    return x + y
}

// Main function with control structures and valid declarations
fun main(args: Array<String>) {
    var a: Int = 10
    val b: Int = 5
    var flag: Boolean = true

    // If-else-if structure
    if (a > b) {
        println("a is greater")
    } else if (a == b) {
        println("a equals b")
    } else {
        println("a is less")
    }

    // While loop
    var count: Int = 0
    while (count < 3) {
        println(count)
        count = count + 1
    }

    // For loop with range
    for (i in 1..3) {
        println(i)
    }

    // Simple return test
    val result: Int = add(a, b)
    println(result)
}