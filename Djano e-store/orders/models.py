from django.db import models
from django.contrib.auth.models import User


# Create your models here.
class Category(models.Model):
    name = models.CharField(max_length=75, unique=True)

    def __str__(self):
        return f"{self.name}"

class Product(models.Model):
    name = models.CharField(max_length=75, unique=True)
    price = models.DecimalField(decimal_places=2, max_digits=4)
    description = models.CharField(max_length=2500, blank=True)

    def __str__(self):
        return f"{self.name}"

class Review(models.Model):
    reviewText = models.CharField(max_length=2500, unique=True)
    product = models.ForeignKey(Product, on_delete=models.CASCADE)
    customer = models.ForeignKey(User, max_length=64, on_delete=models.CASCADE)
    reviewDate = models.DateField()

    def __str__(self):
        return f"{self.customer} {self.product} {self.reviewDate}"

class OrderStatus(models.Model):
    status = models.CharField(max_length=20, unique=True)

    def __str__(self):
        return f"{self.status}"

class Order(models.Model):
    customer = models.ForeignKey(User, max_length=64, on_delete=models.CASCADE)
    orderDate = models.DateField()
    address = models.CharField(max_length=100)
    orderStatus = models.ForeignKey(OrderStatus, on_delete=models.CASCADE)

    def __str__(self):
         return f"{self.customer} {self.orderDate} {self.orderStatus}"

class OrderItem(models.Model):
    product = models.ForeignKey(Product, on_delete=models.CASCADE)
    order = models.ForeignKey(Order, on_delete=models.CASCADE)

    def __str__(self):
        return f"{self.order} {self.product}"
