from django.db import models
from django.core.validators import MaxValueValidator, MinValueValidator
from django.contrib.auth.models import User

# Create your models here.
class Category(models.Model):
    name = models.CharField(max_length=20, unique=True)

    def __str__(self):
        return f"{self.name}"

class AddOnType(models.Model):
    name = models.CharField(max_length=20, unique=True)

    def __str__(self):
        return f"{self.name}"

class AddOn(models.Model):
    name = models.CharField(max_length=20, unique=True)
    addOnType = models.ForeignKey(AddOnType, on_delete=models.CASCADE)

    def __str__(self):
        return f"{self.name} {self.addOnType}"

class Size(models.Model):
    name = models.CharField(max_length=10, unique=True)

    def __str__(self):
        return f"{self.name}"

class AddOnCount(models.Model):
    name=models.CharField(max_length=20, unique=True)
    numberOfAddOns = models.IntegerField(validators=[MaxValueValidator(5), MinValueValidator(0)])
    price = models.DecimalField(decimal_places=2, max_digits=4)
    category = models.ForeignKey(Category, on_delete=models.CASCADE)
    size = models.ForeignKey(Size, blank=True, on_delete=models.CASCADE)
    addOnType = models.ForeignKey(AddOnType, blank=True, on_delete=models.CASCADE)

    def __str__(self):
        return f"{self.name}"

class MenuItem(models.Model):
    name = models.CharField(max_length=25, unique=True, blank=True)
    size = models.ForeignKey(Size, blank=True, on_delete=models.CASCADE)
    price = models.DecimalField(decimal_places=2, max_digits=4)
    category = models.ForeignKey(Category, on_delete=models.CASCADE)
    addOnType = models.ForeignKey(AddOnType, blank=True, on_delete=models.CASCADE)
    maxAddOns = models.IntegerField(validators=[MaxValueValidator(10), MinValueValidator(0)])

    def __str__(self):
        return f"{self.name}"

class OrderStatus(models.Model):
    status = models.CharField(max_length=20, unique=True)

    def __str__(self):
        return f"{self.status}"

class Order(models.Model):
     customer = models.ForeignKey(User, max_length=64, on_delete=models.CASCADE)
     orderDate = models.DateField()
     orderStatus = models.ForeignKey(OrderStatus, on_delete=models.CASCADE)
     #no total needed

     def __str__(self):
         return f"{self.customer} {self.orderDate} {self.orderStatus}"

class OrderItem(models.Model):
    menuItem = models.ForeignKey(MenuItem, on_delete=models.CASCADE)
    addOns = models.ManyToManyField(AddOn, blank=True)
    order = models.ForeignKey(Order, blank=True, on_delete=models.CASCADE)
    total = models.DecimalField(decimal_places=2, max_digits=6)

    def __str__(self):
        return f"{self.menuItem} {self.order}"
