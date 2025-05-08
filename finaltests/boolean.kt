fun main(){
    val truth: Boolean = true
    val falsehood: Boolean = false
    var addition: Int = 1 + 2
    var num: Int = 4
    println("Boolean and Integer tests:\n")
    if (truth) {
        println("True\n")
    } else {
        println("False\n")
    }
    if (falsehood) {
        println("True\n")
    } else {
        println("False\n")
    }
    if (!truth) {
        println("True\n")
    } else {
        println("False\n")
    }
    if(addition == 3){
        println("True\n")
    } else {
        println("False\n")
    }
    if(addition < num){
        println("True\n")
    } else {
        println("False\n")
    }
    if(addition > num){
        println("True\n")
    } else {
        println("False\n")
    }
    if(addition <= num){
        println("True\n")
    } else {
        println("False\n")
    }
}