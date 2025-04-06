// Function returning Unit implicitly
fun logMessage(msg: String) {
    println(msg)
}

// Function returning correct type
fun multiply(x: Int, y: Int): Int {
    return x * y
}

fun describe(flag: Boolean, label: String): String {
    if (flag) {
        return "Yes: " + label
    } else {
        return "No: " + label
    }
}

fun average(x: Int, y: Int): Double {
    val sum: Int = x + y
    val avg: Double = sum / 2.0
    return avg
}

fun tagMessage(name: String, tag: Int): String {
    return name + "#" + tag.toString()
}

fun main(args: Array<String>) {
   val constPi: Double = 3.14
   var counter: Int = 0
   val isReady: Boolean = true
   val title: String = "Test"
   val byteVal: Byte = 127
   val shortVal: Short = 32000
   val longVal: Long = 10000000000
   val floatVal: Float = 2.5f
   val doubleVal: Double = 6.28

   // Test function call correctness
   val result: Double = average(a, b)
   println(result)

   val msg: String = tagMessage("Player", 42)
   println(msg)

   val status: Boolean = isReady && (b > a)
   if (status) {
      println("Ready and valid")
   }

   // Arrays and indexing
   val data: Array<Int>(4) { 0 }
   data[0] = 9
   data[1] = 8
   data[2] = data[0] + data[1]
   println(data[2])

   // Boolean logic and comparisons
   if (data[2] > 10 && status) {
      println("Check passed")
   }

   // for loop with range
   for (i in 1..3) {
      println("Loop index: " + i.toString())
   }

   // Function with no return type
   logMessage("This is a log.")

   // Using nullable operations
   println("Safe length: " + safeLength.toString())
   println("Forced length: " + forcedLength.toString())

   // Use of math and random from standard library
   val rand = java.util.Random()
   println(rand.nextInt())
   println(java.lang.Math.abs(-10))
   println(java.lang.Math.max(3, 7))
   println(java.lang.Math.min(3, 7))
   println(java.lang.Math.pow(2.0, 3.0))
   println(java.lang.Math.sin(0.0))
   println(java.lang.Math.cos(0.0))
   println(java.lang.Math.tan(0.0))
}
