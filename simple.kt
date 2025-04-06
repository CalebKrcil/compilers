fun undeclaredFunction(x: Int): Int{
    return x + 3
}

fun main(){
    val z: Int = undeclaredFunction(3)
    println("wow done")
}